#include "scanner_diagnostic_handler.h"

ScannerDiagnosticHandler::ScannerDiagnosticHandler() : 
    userInterface(UserInterface::getInstance()),
    currentSubMode(0),
    lastSubMode(-1),
    hasCalculatedDifferences(false) {
    // 获取直径扫描仪单例实例
    scanner = DiameterScanner::getInstance();
    encoder = Encoder::getInstance();
    
    // 初始化编码器值数组
    for (int i = 0; i < 4; i++) {
        risingEdgeEncoderValues[i] = 0;
        fallingEdgeEncoderValues[i] = 0;
        diameterDifferences[i] = 0;
        lastSensorStates[i] = false;
        risingEdges[i] = false;
        fallingEdges[i] = false;
        lastRawDiameters[i] = -1; // 初始化为不可能的值，确保首次执行时会输出
    }
    
    // 初始化上一次IO状态为一个不可能的值，确保首次执行时会输出
    lastIOStatus = "";
}

void ScannerDiagnosticHandler::displayRawDiameters() {
    // 生成原始直径值字符串
    String diameters = "D1:" + String(scanner->getHighLevelPulseCount(0)) + 
                      " D2:" + String(scanner->getHighLevelPulseCount(1)) + 
                      " D3:" + String(scanner->getHighLevelPulseCount(2)) + 
                      " D4:" + String(scanner->getHighLevelPulseCount(3));
    
    // 串口输出
    Serial.println("Raw Diameters:");
    for (int i = 0; i < 4; i++) {
        Serial.print("  Scanner ");
        Serial.print(i + 1);
        Serial.print(": ");
        int count = scanner->getHighLevelPulseCount(i);
        float weight = scanner->getSensorWeight(i);
        Serial.print(count);
        Serial.print(" (corrected: ");
        Serial.print(count * weight);
        Serial.println(")");
    }
    
    // OLED显示
    OLED* oled = OLED::getInstance();
    if (oled->isAvailable()) {
        oled->displayDiagnosticInfo("Scanner Diameter", diameters);
    }
}

void ScannerDiagnosticHandler::update() {
    // 检查子模式是否变化
    if (lastSubMode != currentSubMode) {
        lastSubMode = currentSubMode;
    }
    
    // 根据子模式执行相应功能
    if (currentSubMode == 0) {
        // 子模式0：IO状态检查
        // 调用底层DiameterScanner实例获取IO状态数组
        bool* ioStates = scanner->getIOStatusArray();
        
        // 生成状态字符串
        String status = "";
        for (int i = 0; i < 4; i++) {
            status += String(ioStates[i] ? "H" : "L");
            if (i < 3) {
                status += " ";
            }
        }
        
        // 只有当状态发生变化时才输出
        if (status != lastIOStatus) {
            // 串口输出
            Serial.print("IO Status: ");
            Serial.println(status);
            
            // OLED显示
            OLED* oled = OLED::getInstance();
            if (oled->isAvailable()) {
                oled->displayDiagnosticInfo("Scanner IO Status", status);
            }
            
            // 更新上一次状态
            lastIOStatus = status;
        }
    } else if (currentSubMode == 1) {
        // 子模式1：记录并显示传感器上升沿和下降沿的编码器值
        bool* currentStates = scanner->getIOStatusArray();
        
        // 检测上升沿和下降沿，并跟踪是否有变化
        bool hasEdgeChanged = false;
        bool hasFallingEdge = false;
        
        for (int i = 0; i < 4; i++) {
            if (currentStates[i] && !lastSensorStates[i]) {
                // 上升沿
                risingEdges[i] = true;
                // 获取编码器原始计数值并对200取模
                risingEdgeEncoderValues[i] = encoder->getRawCount() % 200;
                // 确保结果为正数
                if (risingEdgeEncoderValues[i] < 0) {
                    risingEdgeEncoderValues[i] += 200;
                }
                hasEdgeChanged = true;
            } else if (!currentStates[i] && lastSensorStates[i]) {
                // 下降沿
                fallingEdges[i] = true;
                // 获取编码器原始计数值并对200取模
                fallingEdgeEncoderValues[i] = encoder->getRawCount() % 200;
                // 确保结果为正数
                if (fallingEdgeEncoderValues[i] < 0) {
                    fallingEdgeEncoderValues[i] += 200;
                }
                
                // 计算并更新直径差值
                int diff = fallingEdgeEncoderValues[i] - risingEdgeEncoderValues[i];
                if (diff < 0) {
                    diff += 200;
                }
                diameterDifferences[i] = diff;
                hasCalculatedDifferences = true;
                
                hasEdgeChanged = true;
                hasFallingEdge = true;
            }
            lastSensorStates[i] = currentStates[i];
        }
        
        // 只有在有上升沿或下降沿变化时才更新显示
        if (hasEdgeChanged) {
            // 生成显示内容
            String encoderInfo = "Encoder Values";
            
            // 串口输出 - 格式化显示
            Serial.println("[DIAGNOSTIC] Encoder Value Recording");
            Serial.println(); // 最头上加一个空行
            
            Serial.print("  Rising edges:");
            for (int i = 0; i < 4; i++) {
                Serial.print(" ");
                if (risingEdgeEncoderValues[i] < 10) {
                    Serial.print("  "); // 两个前导空格
                } else if (risingEdgeEncoderValues[i] < 100) {
                    Serial.print(" ");  // 一个前导空格
                }
                Serial.print(risingEdgeEncoderValues[i]);
            }
            Serial.println();
            
            Serial.print("  Falling edges:");
            for (int i = 0; i < 4; i++) {
                Serial.print(" ");
                if (fallingEdgeEncoderValues[i] < 10) {
                    Serial.print("  "); // 两个前导空格
                } else if (fallingEdgeEncoderValues[i] < 100) {
                    Serial.print(" ");  // 一个前导空格
                }
                Serial.print(fallingEdgeEncoderValues[i]);
            }
            Serial.println();
            
            // 如果已经计算过差值，就显示差值（无论是否有新的下降沿）
            if (hasCalculatedDifferences) {
                Serial.println(); // 差值行前面加一个空行
                // 输出差值：下降沿 - 上升沿（确保非负）
                Serial.print("  Differences:");
                for (int i = 0; i < 4; i++) {
                    Serial.print(" ");
                    if (diameterDifferences[i] < 10) {
                        Serial.print("  "); // 两个前导空格
                    } else if (diameterDifferences[i] < 100) {
                        Serial.print(" ");  // 一个前导空格
                    }
                    Serial.print(diameterDifferences[i]);
                }
                Serial.println();
            }
            
            // OLED显示 - 格式化显示
            OLED* oled = OLED::getInstance();
            if (oled->isAvailable()) {
                // 构建显示内容
                String risingLine = "R:";
                String fallingLine = "F:";
                String diffLine = "D:";
                
                for (int i = 0; i < 4; i++) {
                    // 格式化上升沿值（占3个字符）
                    String risingStr = String(risingEdgeEncoderValues[i]);
                    while (risingStr.length() < 3) risingStr = " " + risingStr;
                    risingLine += risingStr + " ";
                    
                    // 格式化下降沿值（占3个字符）
                    String fallingStr = String(fallingEdgeEncoderValues[i]);
                    while (fallingStr.length() < 3) fallingStr = " " + fallingStr;
                    fallingLine += fallingStr + " ";
                    
                    // 使用存储的差值（占3个字符）
                    String diffStr = String(diameterDifferences[i]);
                    while (diffStr.length() < 3) diffStr = " " + diffStr;
                    diffLine += diffStr + " ";
                }
                
                String oledContent;
                // 如果已经计算过差值，就显示差值（无论是否有新的下降沿）
                if (hasCalculatedDifferences) {
                    // 在上升沿行前和下降沿与差值行之间添加空行
                    oledContent = "\n" + risingLine + "\n" + fallingLine + "\n\n" + diffLine;
                } else {
                    // 没有计算过差值时只显示上升沿和下降沿值
                    oledContent = "\n" + risingLine + "\n" + fallingLine;
                }
                oled->displayDiagnosticInfo(encoderInfo, oledContent);
            }
        }
    } else if (currentSubMode == 2) {
        // 子模式2：显示原始直径
        bool diametersChanged = false;
        int currentDiameters[4];
        
        // 检查直径值是否变化
        for (int i = 0; i < 4; i++) {
            currentDiameters[i] = scanner->getHighLevelPulseCount(i);
            if (currentDiameters[i] != lastRawDiameters[i]) {
                diametersChanged = true;
                lastRawDiameters[i] = currentDiameters[i];
            }
        }
        
        // 只有当直径值变化时才输出
        if (diametersChanged) {
            displayRawDiameters();
        }
    }
}

void ScannerDiagnosticHandler::switchToNextSubMode() {
    // 切换子模式（0 -> 1 -> 2 -> 0）
    currentSubMode = (currentSubMode + 1) % 3;
    
    // 显示当前子模式信息
    Serial.print("[DIAGNOSTIC] Switch to Submode: ");
    switch (currentSubMode) {
        case 0:
            Serial.println("IO Status Check");
            break;
        case 1:
            Serial.println("Encoder Values");
            break;
        case 2:
            Serial.println("Raw Diameter Test");
            break;
        default:
            Serial.println("Unknown");
            break;
    }
}

int ScannerDiagnosticHandler::getCurrentSubMode() const {
    return currentSubMode;
}
