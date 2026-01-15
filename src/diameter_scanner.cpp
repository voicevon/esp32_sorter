#include "diameter_scanner.h"
#include "oled.h"

DiameterScanner::DiameterScanner() : 
    isScanning(false),
    nominalDiameter(0),
    logLevel(LOG_LEVEL_INFO) {
    for (int i = 0; i < 4; i++) {
        scannerPins[i] = LASER_SCANNER_PINS[i];
        highLevelCounts[i] = 0;
        objectCount[i] = 0;
        lastSensorStates[i] = false;
        isSampling[i] = false;
        lastRisingEdge[i] = false;
    }
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
        highLevelCounts[i] = 0;
        objectCount[i] = 0;
        lastSensorStates[i] = false;
        isSampling[i] = false;
        lastRisingEdge[i] = false;
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
            
            if (!isSampling[i]) {
                isSampling[i] = true;
                highLevelCounts[i] = 0;
                lastRisingEdge[i] = true;
            }
            
            highLevelCounts[i]++;
        } else {
            if (isSampling[i]) {
                isSampling[i] = false;
                
                // 当传感器从高电平变为低电平时，计数加一（物体通过）
                if (lastRisingEdge[i]) {
                    objectCount[i]++;
                    lastRisingEdge[i] = false;
                }
            }
        }
        
        lastSensorStates[i] = currentState;
    }
    
    if (anyHigh) {
        if (logLevel == LOG_LEVEL_DEBUG) {
            Serial.print("✗");
        }
    } else {
        if (logLevel == LOG_LEVEL_DEBUG) {
            Serial.print("·");
        }
        
        if (allFinished) {
            float correctedCounts[4];
            int validCount = 0;
            
            for (int i = 0; i < 4; i++) {
                correctedCounts[i] = highLevelCounts[i] * LASER_SCANNER_WEIGHTS[i];
                
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

void DiameterScanner::setLogLevel(LoggerLevel level) {
    logLevel = level;
}

LoggerLevel DiameterScanner::getLogLevel() {
    return logLevel;
}

String DiameterScanner::getIOStatus() {
    bool currentStates[4];
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
    
    // 生成IO状态字符串
    String status = "";
    for (int i = 0; i < 4; i++) {
        status += String(currentStates[i] ? "H" : "L");
        if (i < 3) {
            status += " ";
        }
    }
    
    return status;
}

void DiameterScanner::displayIOStatus() {
    String status = getIOStatus();
    
    // 串口输出
    Serial.print("IO Status: ");
    Serial.println(status);
    
    // OLED显示
    OLED* oled = OLED::getInstance();
    if (oled->isAvailable()) {
        oled->displayDiagnosticInfo("Scanner IO Status", status);
    }
}

String DiameterScanner::getRawDiameters() {
    // 生成原始直径值字符串
    String diameters = "D1:" + String(highLevelCounts[0]) + 
                      " D2:" + String(highLevelCounts[1]) + 
                      " D3:" + String(highLevelCounts[2]) + 
                      " D4:" + String(highLevelCounts[3]);
    
    return diameters;
}

void DiameterScanner::displayRawDiameters() {
    String diameters = getRawDiameters();
    
    // 串口输出
    Serial.println("Raw Diameters:");
    for (int i = 0; i < 4; i++) {
        Serial.print("  Scanner ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(highLevelCounts[i]);
        Serial.print(" (corrected: ");
        Serial.print(highLevelCounts[i] * LASER_SCANNER_WEIGHTS[i]);
        Serial.println(")");
    }
    
    // OLED显示
    OLED* oled = OLED::getInstance();
    if (oled->isAvailable()) {
        oled->displayDiagnosticInfo("Scanner Diameter", diameters);
    }
}