#include "scanner_diagnostic_handler.h"
// #include "user_interface/oled.h"

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
        minRisingEdgeValues[i] = 200; // 初始化为最大值，确保第一个值会被记录为最小值
        maxFallingEdgeValues[i] = 0;  // 初始化为最小值，确保第一个值会被记录为最大值
        diameterDifferences[i] = 0;
        lastSensorStates[i] = false;
        risingEdges[i] = false;
        fallingEdges[i] = false;
        lastRawDiameters[i] = -1; // 初始化为不可能的值，确保首次执行时会输出
        risingEdgeCounts[i] = 0; // 初始化上升沿计数器为0
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
    
    // 串口输出 - 窗口式显示
    static bool firstDisplayRaw = true;
    
    if (firstDisplayRaw) {
        // 第一次显示时，打印六行格式（蓝色背景，红色标题，白色正文）
        Serial.println("\n\033[44m\033[31m      === Raw Diameter Values ===      \033[0m");
        Serial.println("\033[44m\033[37m                                      \033[0m");
        Serial.println("\033[44m\033[37m  Scanner 1: 0 (corrected: 0.0)       \033[0m");
        Serial.println("\033[44m\033[37m  Scanner 2: 0 (corrected: 0.0)       \033[0m");
        Serial.println("\033[44m\033[37m  Scanner 3: 0 (corrected: 0.0)       \033[0m");
        Serial.println("\033[44m\033[37m  Scanner 4: 0 (corrected: 0.0)       \033[0m");
        firstDisplayRaw = false;
    } else {
        // 使用回到行首的方式更新六行数据
        Serial.print("\033[6A"); // 向上移动6行到标题行
        
        // 重新打印标题行（蓝色背景，红色标题）
        Serial.print("\033[44m\033[31m      === Raw Diameter Values ===      \033[0m"); Serial.println();
        
        // 空行
        Serial.println("\033[44m\033[37m                                      \033[0m");
        
        // 更新扫描仪数据行（蓝色背景，白色正文）
        for (int i = 0; i < 4; i++) {
            int count = scanner->getHighLevelPulseCount(i);
            float weight = scanner->getSensorWeight(i);
            float corrected = count * weight;
            
            String line = "  Scanner " + String(i + 1) + ": " + String(count) + " (corrected: " + String(corrected, 1) + ")";
            // 填充空格确保行宽一致
            while (line.length() < 40) {
                line += " ";
            }
            Serial.println("\033[44m\033[37m" + line + "\033[0m");
        }
    }
    
    // OLED显示
    // OLED* oled = OLED::getInstance();
    // if (oled->isAvailable()) {
    //     oled->displayDiagnosticInfo("Scanner Diameter", diameters);
    // }
}

void ScannerDiagnosticHandler::handleIOStatusCheck() {
    // 子模式0：IO状态检查
    // 调用底层DiameterScanner实例获取IO状态数组
    bool* ioStates = scanner->getIOStatusArray();
    
    // 检测上升沿并更新计数器
    bool stateChanged = false;
    for (int i = 0; i < 4; i++) {
        if (ioStates[i] && !lastSensorStates[i]) {
            // 检测到上升沿
            risingEdgeCounts[i]++;
            stateChanged = true;
        }
        lastSensorStates[i] = ioStates[i];
    }
    
    // 生成状态字符串（无分割符，L->LO，H->HI）
    String statusLine = "IO: ";
    for (int i = 0; i < 4; i++) {
        statusLine += String(ioStates[i] ? "HI" : "LO");
        if (i < 3) {
            statusLine += " ";
        }
    }
    
    // 生成计数器字符串（每个三位数字）
    String countLine = "CNT:";
    for (int i = 0; i < 4; i++) {
        String countStr = String(risingEdgeCounts[i]);
        while (countStr.length() < 3) countStr = "0" + countStr;
        countLine += countStr;
        if (i < 3) {
            countLine += " ";
        }
    }
    
    // 组合状态字符串，用于检测变化
    String combinedStatus = statusLine + " | " + countLine;
    
    // 只有当状态发生变化时才输出
    if (combinedStatus != lastIOStatus || stateChanged) {
        // 串口输出 - 窗口式显示
        static bool firstDisplayIO = true;
        
        if (firstDisplayIO) {
            // 第一次显示时，打印三行格式（蓝色背景，红色标题，白色正文）
            Serial.println("\n\033[44m\033[31m        === Scanner IO Status ===        \033[0m");
            Serial.println("\033[44m\033[37m" + statusLine + "                      \033[0m");
            Serial.println("\033[44m\033[37m" + countLine + "                      \033[0m");
            firstDisplayIO = false;
        } else {
            // 使用回到行首的方式更新三行数据
            Serial.print("\033[3A"); // 向上移动3行到标题行
            
            // 重新打印标题行（蓝色背景，红色标题）
            Serial.println("\033[44m\033[31m        === Scanner IO Status ===        \033[0m");
            
            // 更新IO状态数据行（蓝色背景，白色正文）
            Serial.println("\033[44m\033[37m" + statusLine + "                      \033[0m");
            
            // 更新计数器数据行（蓝色背景，白色正文）
            Serial.println("\033[44m\033[37m" + countLine + "                      \033[0m");
        }
        
        // OLED显示
        // OLED* oled = OLED::getInstance();
        // if (oled->isAvailable()) {
        //     // 使用多行显示（使用更新后的statusLine格式）
        //     oled->displayMultiLineText("Scanner IO Status", statusLine, countLine, "", "");
        // }
        
        // 更新上一次状态
        lastIOStatus = combinedStatus;
    }
}

void ScannerDiagnosticHandler::handleEncoderValues() {
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
            
            // 更新最小上升沿值
            if (risingEdgeEncoderValues[i] < minRisingEdgeValues[i]) {
                minRisingEdgeValues[i] = risingEdgeEncoderValues[i];
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
            
            // 更新最大下降沿值
            if (fallingEdgeEncoderValues[i] > maxFallingEdgeValues[i]) {
                maxFallingEdgeValues[i] = fallingEdgeEncoderValues[i];
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
        
        // 串口输出 - 窗口式显示
        static bool firstDisplay = true;
        
        if (firstDisplay) {
            // 第一次显示时，打印六行格式（蓝色背景，红色标题，白色正文）
            Serial.println("\n\033[44m\033[31m    === Encoder Value Recording ===    \033[0m");
            Serial.println("\033[44m\033[37m  Min Rising:      0   0   0   0      \033[0m");
            Serial.println("\033[44m\033[37m  Current Rising:  0   0   0   0      \033[0m");
            Serial.println("\033[44m\033[37m  Current Falling: 0   0   0   0      \033[0m");
            Serial.println("\033[44m\033[37m  Max Falling:     0   0   0   0      \033[0m");
            Serial.println("\033[44m\033[37m  Differences:     0   0   0   0      \033[0m");
            firstDisplay = false;
        } else {
            // 使用回到行首的方式更新六行数据
            Serial.print("\033[6A"); // 向上移动6行到标题行
            
            // 重新打印标题行（蓝色背景，红色标题）
            Serial.print("\033[44m\033[31m    === Encoder Value Recording ===    \033[0m"); Serial.println();
            
            // 更新最小上升沿值行（蓝色背景，白色正文）
            Serial.print("\033[44m\033[37m  Min Rising:");
            for (int i = 0; i < 4; i++) {
                Serial.print(" ");
                if (minRisingEdgeValues[i] < 10) {
                    Serial.print("  "); // 两个前导空格
                } else if (minRisingEdgeValues[i] < 100) {
                    Serial.print(" ");  // 一个前导空格
                }
                Serial.print(minRisingEdgeValues[i]);
            }
            Serial.println("      \033[0m");
            
            // 更新当前上升沿值行（蓝色背景，白色正文）
            Serial.print("\033[44m\033[37m  Current Rising:");
            for (int i = 0; i < 4; i++) {
                Serial.print(" ");
                if (risingEdgeEncoderValues[i] < 10) {
                    Serial.print("  "); // 两个前导空格
                } else if (risingEdgeEncoderValues[i] < 100) {
                    Serial.print(" ");  // 一个前导空格
                }
                Serial.print(risingEdgeEncoderValues[i]);
            }
            Serial.println("      \033[0m");
            
            // 更新当前下降沿值行（蓝色背景，白色正文）
            Serial.print("\033[44m\033[37m  Current Falling:");
            for (int i = 0; i < 4; i++) {
                Serial.print(" ");
                if (fallingEdgeEncoderValues[i] < 10) {
                    Serial.print("  "); // 两个前导空格
                } else if (fallingEdgeEncoderValues[i] < 100) {
                    Serial.print(" ");  // 一个前导空格
                }
                Serial.print(fallingEdgeEncoderValues[i]);
            }
            Serial.println("      \033[0m");
            
            // 更新最大下降沿值行（蓝色背景，白色正文）
            Serial.print("\033[44m\033[37m  Max Falling:");
            for (int i = 0; i < 4; i++) {
                Serial.print(" ");
                if (maxFallingEdgeValues[i] < 10) {
                    Serial.print("  "); // 两个前导空格
                } else if (maxFallingEdgeValues[i] < 100) {
                    Serial.print(" ");  // 一个前导空格
                }
                Serial.print(maxFallingEdgeValues[i]);
            }
            Serial.println("      \033[0m");
            
            // 更新差值行（蓝色背景，白色正文）
            if (hasCalculatedDifferences) {
                Serial.print("\033[44m\033[37m  Differences:");
                for (int i = 0; i < 4; i++) {
                    Serial.print(" ");
                    if (diameterDifferences[i] < 10) {
                        Serial.print("  "); // 两个前导空格
                    } else if (diameterDifferences[i] < 100) {
                        Serial.print(" ");  // 一个前导空格
                    }
                    Serial.print(diameterDifferences[i]);
                }
                Serial.println("      \033[0m");
            } else {
                Serial.println("\033[44m\033[37m  Differences:     0   0   0   0      \033[0m");
            }
        }
        
        // OLED显示 - 格式化显示
        // OLED* oled = OLED::getInstance();
        // if (oled->isAvailable()) {
            // 构建紧凑的显示内容
            String line1 = "Min R:";
            String line2 = "Cur R:";
            String line3 = "Cur F:";
            String line4 = "Max F:";
            
            for (int i = 0; i < 4; i++) {
                // 格式化最小上升沿值（占2个字符）
                String minRisingStr = String(minRisingEdgeValues[i]);
                while (minRisingStr.length() < 2) minRisingStr = " " + minRisingStr;
                line1 += minRisingStr + " ";
                
                // 格式化当前上升沿值（占2个字符）
                String currRisingStr = String(risingEdgeEncoderValues[i]);
                while (currRisingStr.length() < 2) currRisingStr = " " + currRisingStr;
                line2 += currRisingStr + " ";
                
                // 格式化当前下降沿值（占2个字符）
                String currFallingStr = String(fallingEdgeEncoderValues[i]);
                while (currFallingStr.length() < 2) currFallingStr = " " + currFallingStr;
                line3 += currFallingStr + " ";
                
                // 格式化最大下降沿值（占2个字符）
                String maxFallingStr = String(maxFallingEdgeValues[i]);
                while (maxFallingStr.length() < 2) maxFallingStr = " " + maxFallingStr;
                line4 += maxFallingStr + " ";
            }
            
            // 使用displayMultiLineText方法，显示所有四行数据
            userInterface->displayMultiLineText(encoderInfo, line1, line2, line3, line4);
        // }
    }
}

void ScannerDiagnosticHandler::handleRawDiameterDisplay() {
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

void ScannerDiagnosticHandler::update() {
    // 检查子模式是否变化
    if (lastSubMode != currentSubMode) {
        lastSubMode = currentSubMode;
    }
    
    // 根据子模式调用相应的处理函数
    switch (currentSubMode) {
        case 0:
            handleIOStatusCheck();
            break;
        case 1:
            handleEncoderValues();
            break;
        case 2:
            handleRawDiameterDisplay();
            break;
    }
}

void ScannerDiagnosticHandler::switchToNextSubMode() {
    // 切换子模式（0 -> 1 -> 2 -> 0）
    currentSubMode = (currentSubMode + 1) % 3;
    
    // 如果切换到IO状态检查子模式，将上升沿计数器清零
    if (currentSubMode == 0) {
        for (int i = 0; i < 4; i++) {
            risingEdgeCounts[i] = 0;
        }
        // 重置IO状态，确保下次进入时会重新显示
        lastIOStatus = "";
    }
    
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
