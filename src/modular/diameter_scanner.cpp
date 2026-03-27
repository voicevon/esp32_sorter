#include "diameter_scanner.h"
// #include "user_interface/oled.h"

// 初始化静态实例变量为NULL
// DiameterScanner* DiameterScanner::instance = NULL; // Managed by Singleton template

DiameterScanner::DiameterScanner() : 
    isScanning(false),
    nominalDiameter(0) {
    for (int i = 0; i < 4; i++) {
        scannerPins[i] = PINS_SCANNER[i];
        highLevelPulseCounts[i] = 0;
        objectCount[i] = 0;
        lastSensorStates[i] = false;
        isObjectPassing[i] = false;
    }
    lastPhase = -1;
}

// 实现单例模式的getInstance方法 - Managed by Singleton template
// DiameterScanner* DiameterScanner::getInstance() { ... }

void DiameterScanner::initialize() {
    for (int i = 0; i < 4; i++) {
        pinMode(scannerPins[i], INPUT);
    }
    
    // 移除 initialize() 中的 start() 调用，确保开机时处于受控的静默状态。
    // 真正的扫描开启将由 Sorter 在探测到 Phase 50 时触发。
    
    Serial.println("DiameterScanner initialized (Silent)");
}

void DiameterScanner::start() {
    isScanning = true;
    nominalDiameter = 0;
    sampleCount = 0;
    for (int i = 0; i < 4; i++) {
        highLevelPulseCounts[i] = 0;
        objectCount[i] = 0;
        lastSensorStates[i] = false;
        isObjectPassing[i] = false;
    }
    lastPhase = -1;
}

void DiameterScanner::stop() {
    isScanning = false;
}

void DiameterScanner::sample(int phase) {
    if (!isScanning) return;
    
    // 终极同步过滤器：只有当相位确实"向前进"时，才进行采样。
    // 修复：回绕检测不依赖固定的 199->0，而是检测任意大幅负跳变（超过半圈视为合法回绕）
    bool isForward = false;
    if (lastPhase == -1) {
        isForward = true; // 第一次采样，无条件允许
    } else if (phase > lastPhase) {
        isForward = true; // 正常前进
    } else if ((lastPhase - phase) > (ENCODER_MAX_PHASE / 2)) {
        isForward = true; // 合法回绕（跨越半圈以上的负跳变）
    }
    
    if (!isForward) return; // 过滤震动回弹或重复触发
    lastPhase = phase;
    
    // 仅通过数组记录采样点的原始高低电平状态，极大地降低中断开销
    if (sampleCount < MAX_SAMPLES) {
        for (int i = 0; i < 4; i++) {
            sensorBuffers[i][sampleCount] = (digitalRead(scannerPins[i]) == HIGH) ? 1 : 0;
        }
        sampleCount++;
    }
}

int DiameterScanner::getDiameterAndStop() {
    // 必须首先调用 stop()，确保在后续计算过程中不再有中断累加。
    // 在 Sorter::onPhaseChange 中理应已调用过一次 stop()，此处为双重保险。
    stop(); 
    
    // 初始化计算状态变量，清零旧数据
    for (int i = 0; i < 4; i++) {
        highLevelPulseCounts[i] = 0;
        objectCount[i] = 0;
        lastSensorStates[i] = false;
        isObjectPassing[i] = false;
    }

    // 后期处理：遍历缓存区，回放采样历史，计算有效高电平脉冲计数
    for (int idx = 0; idx < sampleCount; idx++) {
        for (int i = 0; i < 4; i++) {
            bool currentState = (sensorBuffers[i][idx] == 1);
            
            if (currentState) {
                if (!isObjectPassing[i]) {
                    isObjectPassing[i] = true;
                    highLevelPulseCounts[i] = 0; // 一旦新物体遮挡，重新计数
                }
                highLevelPulseCounts[i]++;
            } else {
                if (isObjectPassing[i]) {
                    isObjectPassing[i] = false;
                    // 当传感器从高电平变为低电平时，计数加一（物体通过）
                    objectCount[i]++;
                }
            }
        }
    }
    
    // [DIAGNOSTIC LOG] 输出原始计数值和缓冲区大小
    Serial.printf("[SCANNER_DEBUG] Raw Counts: CH0:%d, CH1:%d, CH2:%d, CH3:%d | LastPhase:%d, Samples:%d\n", 
                  highLevelPulseCounts[0], highLevelPulseCounts[1], highLevelPulseCounts[2], highLevelPulseCounts[3], lastPhase, sampleCount);
    
    //在此处执行计算逻辑
    float correctedCounts[2];
    int validCount = 0;
            
    for (int i = 0; i < 2; i++) {
        correctedCounts[i] = highLevelPulseCounts[i] * SCANNER_WEIGHTS[i];
        
        if (correctedCounts[i] >= SCANNER_MIN_DIAMETER_UNIT) {
            validCount++;
        }
    }
            
    if (validCount > 0) {
        float validValues[2];
        int validIndex = 0;
        
        for (int i = 0; i < 2; i++) {
            if (correctedCounts[i] >= SCANNER_MIN_DIAMETER_UNIT) {
                validValues[validIndex++] = correctedCounts[i];
            }
        }
        
        if (validCount == 1) {
            nominalDiameter = (int)(validValues[0] + 0.5f);
        } else if (validCount == 2) {
            // 取平均值
            nominalDiameter = (int)((validValues[0] + validValues[1]) / 2.0f + 0.5f);
        }
    } else {
        nominalDiameter = 0;
    }

    return nominalDiameter;
}

// 获取物体的长度级别 (1:S, 2:M, 3:L)
int DiameterScanner::getLengthLevel() {
    // 门限设定：5个脉冲约为 2.5mm，低于此值的触发视为干扰噪声
    const int LENGTH_NOISE_THRESHOLD = 5;
    
    // 如果最远端传感器（Index 3）被覆盖，确认为 L (长)
    if (highLevelPulseCounts[3] >= LENGTH_NOISE_THRESHOLD) return 3;
    
    // 如果中端传感器（Index 2）被覆盖，确认为 M (中)
    if (highLevelPulseCounts[2] >= LENGTH_NOISE_THRESHOLD) return 2;
    
    // 否则为 S (短)
    return 1;
}

int DiameterScanner::getObjectCount(int index) const {
    if (index >= 0 && index < 4) {
        return objectCount[index];
    }
    return 0;
}

int DiameterScanner::getTotalObjectCount() const {
    int total = 0;
    for (int i = 0; i < 4; i++) {
        total += objectCount[i];
    }
    return total;
}

bool* DiameterScanner::getIOStatusArray() {
    static bool currentStates[4];
    bool stateChanged = false;
    
    // 读取所有4个传感器的当前状态
    for (int i = 0; i < 4; i++) {
        currentStates[i] = (digitalRead(scannerPins[i]) == HIGH);
        
        // 检查状态是否变化
        if (currentStates[i] != lastSensorStates[i]) {
            stateChanged = true;
        }
    }
    
    // 更新最后状态
    if (stateChanged) {
        for (int i = 0; i < 4; i++) {
            lastSensorStates[i] = currentStates[i];
        }
    }
    
    return currentStates;
}

int DiameterScanner::getHighLevelPulseCount(int index) const {
    if (index >= 0 && index < 4) {
        return highLevelPulseCounts[index];
    }
    return 0;
}

float DiameterScanner::getSensorWeight(int index) const {
    if (index >= 0 && index < 4) {
        return SCANNER_WEIGHTS[index];
    }
    return 0.0f;
}