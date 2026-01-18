#include "scanner_diagnostic_handler.h"

ScannerDiagnosticHandler::ScannerDiagnosticHandler() : 
    userInterface(UserInterface::getInstance()),
    currentSubMode(0),
    lastSubMode(-1) {
    // 获取直径扫描仪单例实例
    scanner = DiameterScanner::getInstance();
    encoder = Encoder::getInstance();
    
    // 初始化编码器值数组
    for (int i = 0; i < 4; i++) {
        risingEdgeEncoderValues[i] = 0;
        fallingEdgeEncoderValues[i] = 0;
        lastSensorStates[i] = false;
        risingEdges[i] = false;
        fallingEdges[i] = false;
    }
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
        
        // 串口输出
        Serial.print("IO Status: ");
        Serial.println(status);
        
        // OLED显示
        OLED* oled = OLED::getInstance();
        if (oled->isAvailable()) {
            oled->displayDiagnosticInfo("Scanner IO Status", status);
        }
    } else if (currentSubMode == 1) {
        // 子模式1：记录并显示传感器上升沿和下降沿的编码器值
        bool* currentStates = scanner->getIOStatusArray();
        
        // 检测上升沿和下降沿，并跟踪是否有变化
        bool hasEdgeChanged = false;
        
        for (int i = 0; i < 4; i++) {
            if (currentStates[i] && !lastSensorStates[i]) {
                // 上升沿
                risingEdges[i] = true;
                risingEdgeEncoderValues[i] = encoder->getRawCount(); // 获取当前编码器原始计数值
                hasEdgeChanged = true;
            } else if (!currentStates[i] && lastSensorStates[i]) {
                // 下降沿
                fallingEdges[i] = true;
                fallingEdgeEncoderValues[i] = encoder->getRawCount(); // 获取当前编码器原始计数值
                hasEdgeChanged = true;
            }
            lastSensorStates[i] = currentStates[i];
        }
        
        // 只有在有上升沿或下降沿变化时才更新显示
        if (hasEdgeChanged) {
            // 生成显示内容
            String encoderInfo = "Encoder Values";
            
            // 串口输出
            Serial.println("[DIAGNOSTIC] Encoder Value Recording");
            for (int i = 0; i < 4; i++) {
                Serial.print("  Sensor ");
                Serial.print(i + 1);
                Serial.print(": Rising=");
                Serial.print(risingEdgeEncoderValues[i]);
                Serial.print(", Falling=");
                Serial.print(fallingEdgeEncoderValues[i]);
                Serial.println();
            }
            
            // OLED显示
            OLED* oled = OLED::getInstance();
            if (oled->isAvailable()) {
                String oledContent = "";
                for (int i = 0; i < 4; i++) {
                    oledContent += "S" + String(i + 1) + ":" + String(risingEdgeEncoderValues[i]) + "/" + String(fallingEdgeEncoderValues[i]) + "\n";
                }
                oled->displayDiagnosticInfo(encoderInfo, oledContent);
            }
        }
    } else if (currentSubMode == 2) {
        // 子模式2：显示原始直径
        displayRawDiameters();
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
