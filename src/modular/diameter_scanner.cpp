#include "diameter_scanner.h"
#include "user_interface/oled.h"

// 初始化静态实例变量为NULL
DiameterScanner* DiameterScanner::instance = NULL;

DiameterScanner::DiameterScanner() : 
    isScanning(false),
    nominalDiameter(0) {
    for (int i = 0; i < 4; i++) {
        scannerPins[i] = LASER_SCANNER_PINS[i];
        highLevelPulseCounts[i] = 0;
        objectCount[i] = 0;
        lastSensorStates[i] = false;
        isObjectPassing[i] = false;
    }
}

// 实现单例模式的getInstance方法
DiameterScanner* DiameterScanner::getInstance() {
    if (instance == NULL) {
        instance = new DiameterScanner();
    }
    return instance;
}

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
    
    if (!anyHigh && allFinished) {
            float correctedCounts[4];
            int validCount = 0;
            
            for (int i = 0; i < 4; i++) {
                correctedCounts[i] = highLevelPulseCounts[i] * LASER_SCANNER_WEIGHTS[i];
                
                if (correctedCounts[i] >= LASER_SCANNER_MIN_DIAMETER) {
                    validCount++;
                }
            }
            
            if (validCount >= 2) {
                float validValues[4];
                int validIndex = 0;
                
                for (int i = 0; i < 4; i++) {
                    if (correctedCounts[i] >= LASER_SCANNER_MIN_DIAMETER) {
                        validValues[validIndex++] = correctedCounts[i];
                    }
                }
                
                for (int i = 0; i < validCount - 1; i++) {
                    for (int j = i + 1; j < validCount; j++) {
                        if (validValues[i] > validValues[j]) {
                            float temp = validValues[i];
                            validValues[i] = validValues[j];
                            validValues[j] = temp;
                        }
                    }
                }
                
                if (validCount == 2) {
                    nominalDiameter = (int)((validValues[0] + validValues[1]) / 2);
                } else if (validCount == 3) {
                    nominalDiameter = (int)validValues[1];
                } else {
                    nominalDiameter = (int)((validValues[1] + validValues[2]) / 2);
                }
            }
        }
}

int DiameterScanner::getDiameterAndStop() const {
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
        return LASER_SCANNER_WEIGHTS[index];
    }
    return 0.0f;
}