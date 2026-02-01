#include "diameter_scanner.h"
#include "user_interface/oled.h"

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
}

// 实现单例模式的getInstance方法 - Managed by Singleton template
// DiameterScanner* DiameterScanner::getInstance() { ... }

void DiameterScanner::initialize() {
    for (int i = 0; i < 4; i++) {
        pinMode(scannerPins[i], INPUT);
    }
    
    start();
    
    Serial.println("DiameterScanner initialized");
}

void DiameterScanner::start() {
    isScanning = true;
    nominalDiameter = 0;
    for (int i = 0; i < 4; i++) {
        highLevelPulseCounts[i] = 0;
        objectCount[i] = 0;
        lastSensorStates[i] = false;
        isObjectPassing[i] = false;
    }
}

void DiameterScanner::sample(int phase) {
    if (!isScanning) return;
    
    bool anyHigh = false;
    bool allFinished = true;
    
    for (int i = 0; i < 4; i++) {
        bool currentState = (digitalRead(scannerPins[i]) == HIGH);
        
        if (currentState) {
            anyHigh = true;
            allFinished = false;
            
            if (!isObjectPassing[i]) {
                isObjectPassing[i] = true;
                highLevelPulseCounts[i] = 0;
            }
            
            highLevelPulseCounts[i]++;
        } else {
            if (isObjectPassing[i]) {
                isObjectPassing[i] = false;
                
                // 当传感器从高电平变为低电平时，计数加一（物体通过）
                objectCount[i]++;
            }
        }
        
        lastSensorStates[i] = currentState;
    }
    
    // 仅负责状态检测和计数
    if (!anyHigh && allFinished) {
        // 之前在此处的计算逻辑已移至 getDiameterAndStop()
        // 此处只负责检测结束状态（如果需要）
    }
}

int DiameterScanner::getDiameterAndStop() {
    isScanning = false; // 停止扫描
    
    //在此处执行计算逻辑
    float correctedCounts[4];
    int validCount = 0;
            
    for (int i = 0; i < 4; i++) {
        correctedCounts[i] = highLevelPulseCounts[i] * SCANNER_WEIGHTS[i];
        
        if (correctedCounts[i] >= SCANNER_MIN_DIAMETER_UNIT) {
            validCount++;
        }
    }
            
    if (validCount >= 2) {
        float validValues[4];
        int validIndex = 0;
        
        for (int i = 0; i < 4; i++) {
            if (correctedCounts[i] >= SCANNER_MIN_DIAMETER_UNIT) {
                validValues[validIndex++] = correctedCounts[i];
            }
        }
        
        // 冒泡排序
        for (int i = 0; i < validCount - 1; i++) {
            for (int j = i + 1; j < validCount; j++) {
                if (validValues[i] > validValues[j]) {
                    float temp = validValues[i];
                    validValues[i] = validValues[j];
                    validValues[j] = temp;
                }
            }
        }
        
        // 根据有效值的数量计算标称直径
        if (validCount == 2) {
            nominalDiameter = (int)((validValues[0] + validValues[1]) / 2);
        } else if (validCount == 3) {
            nominalDiameter = (int)validValues[1]; // 取中间值
        } else {
            nominalDiameter = (int)((validValues[1] + validValues[2]) / 2); // 去掉最大最小，取中间平均
        }
    } else {
        nominalDiameter = 0;
    }

    return nominalDiameter;
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