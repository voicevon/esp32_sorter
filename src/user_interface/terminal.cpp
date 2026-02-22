#include "terminal.h"

// 初始化静态实例指针
Terminal* Terminal::instance = nullptr;

// 私有构造函数实现
Terminal::Terminal() {
    // 初始化串口
    Serial.begin(115200);
    
    // 等待串口初始化完成
    delay(100);
    
    // 初始化上次更新时间
    previousUpdateTime = 0;
    
    // 初始化上一次显示的数据
    previousDisplayedMode = MODE_NORMAL;
    previousEncoderPosition = 0;
    previousSortingSpeedPerSecond = 0.0;
    previousSortingSpeedPerMinute = 0;
    previousSortingSpeedPerHour = 0;
    previousIdentifiedCount = 0;
    previousTransportedTrayCount = 0;
    previousLatestDiameter = 0;
}

// 单例模式获取实例
Terminal* Terminal::getInstance() {
    if (instance == nullptr) {
        instance = new Terminal();
    }
    return instance;
}

// 初始化串口终端
void Terminal::initialize() {
    // 串口已经在构造函数中初始化
    Serial.println("Terminal initialized");
}

// 检查是否可以更新显示
bool Terminal::isUpdateReady() const {
    return millis() - previousUpdateTime >= UPDATE_INTERVAL;
}

// 翻译功能（简单实现，返回原文）
String Terminal::translate(const String& key) const {
    // 这里可以扩展为支持多语言
    return key;
}

// 显示模式变化信息（使用SystemMode枚举）
void Terminal::displayModeChange(SystemMode newMode) {
    String modeName;
    switch (newMode) {
        case MODE_NORMAL:
            modeName = "Normal Mode";
            break;
        case MODE_DIAGNOSE_ENCODER:
            modeName = "Encoder Diagnostics";
            break;
        case MODE_DIAGNOSE_SCANNER:
            modeName = "Scanner Diagnostics";
            break;
        case MODE_DIAGNOSE_OUTLET:
            modeName = "Outlet Diagnostics";
            break;
        case MODE_VERSION_INFO:
            modeName = "Version Information";
            break;
        default:
            modeName = "Unknown Mode";
            break;
    }
    displayModeChange(modeName);
}

// 显示模式变化信息（使用字符串）
void Terminal::displayModeChange(const String& newModeName) {
    // 显示模式变化信息（使用通知消息样式）
    Serial.println("\n" + STYLE_NOTIFICATION + "        === Mode Change ===        " + STYLE_RESET);
    Serial.print(STYLE_NOTIFICATION + "Mode changed to: " + newModeName + "                      " + STYLE_RESET);
    Serial.println();
    
    // 如果是子模式切换，添加额外提示
    if (newModeName.indexOf("Submode") != -1) {
        Serial.println(STYLE_NOTIFICATION + "Use slave button to switch submode               " + STYLE_RESET);
    }
}

// 显示出口状态变化
void Terminal::displayOutletStatus(uint8_t outletIndex, bool isOpen) {
    Serial.print("Outlet ");
    Serial.print(outletIndex);
    Serial.print(": ");
    Serial.println(isOpen ? "Open" : "Closed");
}

// 显示诊断信息
void Terminal::displayDiagnosticInfo(const String& title, const String& info) {
    // 检查是否是第一次显示
    static bool firstDisplay = true;
    
    if (firstDisplay) {
        // 第一次显示时，打印两行格式（蓝色背景，红色标题，白色正文）
        Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "        === " + title + " ===        " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + info + "                      " + STYLE_RESET);
        firstDisplay = false;
    } else {
        // 使用回到行首的方式更新两行数据
        Serial.print("\033[2A"); // 向上移动2行到标题行
        
        // 重新打印标题行（蓝色背景，红色标题）
        Serial.print(STYLE_DATA_WINDOW_TITLE + "        === " + title + " ===        " + STYLE_RESET); Serial.println();
        
        // 更新数据行（蓝色背景，白色正文）
        Serial.println(STYLE_DATA_WINDOW_CONTENT + info + "                      " + STYLE_RESET);
    }
}

// 显示出口测试模式图形
void Terminal::displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) {
    String subModeName = (subMode == 0) ? "Cycle Drop" : "Cycle Raise";
    
    // 检查是否是第一次显示
    static bool firstDisplay = true;
    
    if (firstDisplay) {
        // 第一次显示时，打印四行格式（蓝色背景，红色标题，白色正文）
        Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "      === Outlet Test Mode ===      " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + "Outlet Count: 0                     " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + "Open Outlet: 0                      " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + "Submode: Cycle Drop                 " + STYLE_RESET);
        firstDisplay = false;
    } else {
        // 使用回到行首的方式更新四行数据
        Serial.print("\033[4A"); // 向上移动4行到标题行
        
        // 重新打印标题行（蓝色背景，红色标题）
        Serial.print(STYLE_DATA_WINDOW_TITLE + "      === Outlet Test Mode ===      " + STYLE_RESET); Serial.println();
        
        // 更新出口数量行（蓝色背景，白色正文）
        Serial.print(STYLE_DATA_WINDOW_CONTENT + "Outlet Count: ");
        Serial.print(outletCount);
        Serial.print("                     " + STYLE_RESET); Serial.println();
        
        // 更新打开的出口行（蓝色背景，白色正文）
        Serial.print(STYLE_DATA_WINDOW_CONTENT + "Open Outlet: ");
        Serial.print(openOutlet);
        Serial.print("                      " + STYLE_RESET); Serial.println();
        
        // 更新子模式行（蓝色背景，白色正文）
        Serial.print(STYLE_DATA_WINDOW_CONTENT + "Submode: ");
        Serial.print(subModeName);
        Serial.print("                 " + STYLE_RESET); Serial.println();
    }
}

// 显示出口寿命测试专用图形（三行简洁版本）
void Terminal::displayOutletLifetimeTestGraphic(uint8_t outletCount, unsigned long cycleCount, bool outletState, int subMode) {
    if (subMode == 2) {
        // 检查是否是第一次显示
        static bool firstDisplay = true;
        
        if (firstDisplay) {
            // 第一次显示时，打印三行格式（蓝色背景，红色标题，白色正文）
            Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "      === Servo Lifetime Test ===      " + STYLE_RESET);
            Serial.println(STYLE_DATA_WINDOW_CONTENT + "Outlet Count: 0 | Cycle: 0             " + STYLE_RESET);
            Serial.println(STYLE_DATA_WINDOW_CONTENT + "State: Closed                          " + STYLE_RESET);
            firstDisplay = false;
        } else {
            // 使用回到行首的方式更新三行数据
            Serial.print("\033[3A"); // 向上移动3行到标题行
            
            // 重新打印标题行（蓝色背景，红色标题）
            Serial.print(STYLE_DATA_WINDOW_TITLE + "      === Servo Lifetime Test ===      " + STYLE_RESET); Serial.println();
            
            // 更新出口和循环数据行（蓝色背景，白色正文）
            Serial.print(STYLE_DATA_WINDOW_CONTENT + "Outlet Count: ");
            Serial.print(outletCount);
            Serial.print(" | Cycle: ");
            Serial.print(cycleCount);
            Serial.print("             " + STYLE_RESET); Serial.println();
            
            // 更新状态数据行（蓝色背景，白色正文）
            Serial.print(STYLE_DATA_WINDOW_CONTENT + "State: ");
            Serial.print(outletState ? "Open" : "Closed");
            Serial.print("                          " + STYLE_RESET); Serial.println();
        }
    }
}

// 显示扫描仪编码器值
void Terminal::displayScannerEncoderValues(const int* risingValues, const int* fallingValues) {
    // 检查是否是第一次显示
    static bool firstDisplay = true;
    
    if (firstDisplay) {
        // 第一次显示时，打印三行格式（蓝色背景，红色标题，白色正文）
        Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "    === Scanner Encoder Values ===    " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + "Rising: 0 0 0 0 0 0 0 0             " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + "Falling: 0 0 0 0 0 0 0 0            " + STYLE_RESET);
        firstDisplay = false;
    } else {
        // 使用回到行首的方式更新三行数据
        Serial.print("\033[3A"); // 向上移动3行到标题行
        
        // 重新打印标题行（蓝色背景，红色标题）
        Serial.print(STYLE_DATA_WINDOW_TITLE + "    === Scanner Encoder Values ===    " + STYLE_RESET); Serial.println();
        
        // 更新上升沿值行（蓝色背景，白色正文）
        Serial.print(STYLE_DATA_WINDOW_CONTENT + "Rising: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(risingValues[i]);
            Serial.print(" ");
        }
        Serial.print("             " + STYLE_RESET); Serial.println();
        
        // 更新下降沿值行（蓝色背景，白色正文）
        Serial.print(STYLE_DATA_WINDOW_CONTENT + "Falling: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(fallingValues[i]);
            Serial.print(" ");
        }
        Serial.print("            " + STYLE_RESET); Serial.println();
    }
}

// 显示系统仪表盘（三行简洁版本）
void Terminal::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    if (isUpdateReady() || 
        sortingSpeedPerSecond != previousSortingSpeedPerSecond ||
        sortingSpeedPerMinute != previousSortingSpeedPerMinute ||
        sortingSpeedPerHour != previousSortingSpeedPerHour ||
        identifiedCount != previousIdentifiedCount ||
        transportedTrayCount != previousTransportedTrayCount) {
        
        // 检查是否是第一次显示
        static bool firstDisplay = true;
        
        if (firstDisplay) {
            // 第一次显示时，打印三行格式（蓝色背景，红色标题，白色正文）
            // 固定宽度为29字符，确保长方形窗口
            Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "       === System Dashboard ===       " + STYLE_RESET);
            Serial.println(STYLE_DATA_WINDOW_CONTENT + "Speed:     0.0/s, 0/min, 0/h          " + STYLE_RESET);
            Serial.println(STYLE_DATA_WINDOW_CONTENT + "Identified: 0 | Transported: 0         " + STYLE_RESET);
            firstDisplay = false;
        } else {
            // 使用回到行首的方式更新三行数据
            Serial.print("\033[3A"); // 向上移动3行到标题行
            
            // 重新打印标题行（蓝色背景，红色标题） - 居中且保持37字符宽度
            Serial.print(STYLE_DATA_WINDOW_TITLE + "       === System Dashboard ===       " + STYLE_RESET); Serial.println();
            
            // 更新速度数据行（蓝色背景，白色正文） - 保持37字符宽度
            Serial.print(STYLE_DATA_WINDOW_CONTENT + "Speed:     ");
            Serial.print(sortingSpeedPerSecond, 1);
            Serial.print("/s, ");
            Serial.print(sortingSpeedPerMinute);
            Serial.print("/min, ");
            Serial.print(sortingSpeedPerHour);
            Serial.print("/h");
            Serial.print("          "); // 填充空格确保37字符宽度
            Serial.println(STYLE_RESET);
            
            // 更新计数数据行（蓝色背景，白色正文） - 保持37字符宽度
            Serial.print(STYLE_DATA_WINDOW_CONTENT + "Identified: ");
            Serial.print(identifiedCount);
            Serial.print(" | Transported: ");
            Serial.print(transportedTrayCount);
            Serial.print("         "); // 填充空格确保37字符宽度
            Serial.println(STYLE_RESET);
        }
        
        // 更新上次显示的数据
        previousSortingSpeedPerSecond = sortingSpeedPerSecond;
        previousSortingSpeedPerMinute = sortingSpeedPerMinute;
        previousSortingSpeedPerHour = sortingSpeedPerHour;
        previousIdentifiedCount = identifiedCount;
        previousTransportedTrayCount = transportedTrayCount;
        
        // 更新上次更新时间
        previousUpdateTime = millis();
    }
}

// 显示直径信息（功能专用方法）
void Terminal::displayDiameter(int latestDiameter) {
    if (isUpdateReady() || latestDiameter != previousLatestDiameter) {
        // 检查是否是第一次显示
        static bool firstDisplay = true;
        
        if (firstDisplay) {
            // 第一次显示时，打印两行格式（蓝色背景，红色标题，白色正文）
            Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "        === Diameter Display ===        " + STYLE_RESET);
            Serial.println(STYLE_DATA_WINDOW_CONTENT + "Current Diameter: 0 mm                  " + STYLE_RESET);
            firstDisplay = false;
        } else {
            // 使用回到行首的方式更新两行数据
            Serial.print("\033[2A"); // 向上移动2行到标题行
            
            // 重新打印标题行（蓝色背景，红色标题）
            Serial.print(STYLE_DATA_WINDOW_TITLE + "        === Diameter Display ===        " + STYLE_RESET); Serial.println();
            
            // 更新直径数据行（蓝色背景，白色正文）
            Serial.print(STYLE_DATA_WINDOW_CONTENT + "Current Diameter: ");
            Serial.print(latestDiameter);
            Serial.print(" mm");
            Serial.print("                  " + STYLE_RESET); Serial.println();
        }
        
        previousLatestDiameter = latestDiameter;
        previousUpdateTime = millis();
    }
}

// 显示正常模式直径信息（兼容旧接口）
void Terminal::displayNormalModeDiameter(int latestDiameter) {
    displayDiameter(latestDiameter);
}

// 显示正常模式统计信息（保持兼容）
void Terminal::displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    displayDashboard(sortingSpeedPerSecond, sortingSpeedPerMinute, sortingSpeedPerHour, identifiedCount, transportedTrayCount);
}

// 显示速度统计信息
void Terminal::displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) {
    if (isUpdateReady()) {
        // 检查是否是第一次显示
        static bool firstDisplay = true;
        
        if (firstDisplay) {
            // 第一次显示时，打印四行格式（蓝色背景，红色标题，白色正文）
            Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "      === Speed Statistics ===      " + STYLE_RESET);
            Serial.println(STYLE_DATA_WINDOW_CONTENT + "Speed: 0/s, 0/min, 0/h              " + STYLE_RESET);
            Serial.println(STYLE_DATA_WINDOW_CONTENT + "Items Processed: 0                  " + STYLE_RESET);
            Serial.println(STYLE_DATA_WINDOW_CONTENT + "Trays Transported: 0                " + STYLE_RESET);
            firstDisplay = false;
        } else {
            // 使用回到行首的方式更新四行数据
            Serial.print("\033[4A"); // 向上移动4行到标题行
            
            // 重新打印标题行（蓝色背景，红色标题）
            Serial.print(STYLE_DATA_WINDOW_TITLE + "      === Speed Statistics ===      " + STYLE_RESET); Serial.println();
            
            // 更新速度数据行（蓝色背景，白色正文）
            Serial.print(STYLE_DATA_WINDOW_CONTENT + "Speed: ");
            Serial.print(speedPerSecond);
            Serial.print("/s, ");
            Serial.print(speedPerMinute);
            Serial.print("/min, ");
            Serial.print(speedPerHour);
            Serial.print("/h");
            Serial.print("              " + STYLE_RESET); Serial.println();
            
            // 更新处理的项目数行（蓝色背景，白色正文）
            Serial.print(STYLE_DATA_WINDOW_CONTENT + "Items Processed: ");
            Serial.print(itemCount);
            Serial.print("                  " + STYLE_RESET); Serial.println();
            
            // 更新运输的托盘数行（蓝色背景，白色正文）
            Serial.print(STYLE_DATA_WINDOW_CONTENT + "Trays Transported: ");
            Serial.print(trayCount);
            Serial.print("                " + STYLE_RESET); Serial.println();
        }
        
        previousUpdateTime = millis();
    }
}

// 显示单个值
void Terminal::displaySingleValue(const String& label, int value, const String& unit) {
    // 检查是否是第一次显示
    static bool firstDisplay = true;
    
    if (firstDisplay) {
        // 第一次显示时，打印两行格式（蓝色背景，红色标题，白色正文）
        Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "        === Sensor Reading ===        " + STYLE_RESET);
        Serial.print(STYLE_DATA_WINDOW_CONTENT + label + ": ");
        Serial.print(value);
        if (!unit.isEmpty()) {
            Serial.print(" ");
            Serial.print(unit);
        }
        Serial.println("                      " + STYLE_RESET);
        firstDisplay = false;
    } else {
        // 使用回到行首的方式更新两行数据
        Serial.print("\033[2A"); // 向上移动2行到标题行
        
        // 重新打印标题行（蓝色背景，红色标题）
        Serial.print(STYLE_DATA_WINDOW_TITLE + "        === Sensor Reading ===        " + STYLE_RESET); Serial.println();
        
        // 更新数据行（蓝色背景，白色正文）
        Serial.print(STYLE_DATA_WINDOW_CONTENT + label + ": ");
        Serial.print(value);
        if (!unit.isEmpty()) {
            Serial.print(" ");
            Serial.print(unit);
        }
        Serial.println("                      " + STYLE_RESET);
    }
}

// 显示位置信息
void Terminal::displayPositionInfo(const String& title, int position, bool showOnlyOnChange) {
    if (showOnlyOnChange && position == previousEncoderPosition) {
        return;
    }
    
    // 检查是否是第一次显示
    static bool firstDisplay = true;
    
    if (firstDisplay) {
        // 第一次显示时，打印两行格式（蓝色背景，红色标题，白色正文）
        Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "        === Encoder Diagnostics ===        " + STYLE_RESET);
        Serial.print(STYLE_DATA_WINDOW_CONTENT);
        Serial.print(title);
        Serial.print(": Position = 0");
        Serial.println("                      " + STYLE_RESET);
        firstDisplay = false;
    } else {
        // 使用回到行首的方式更新两行数据
        Serial.print("\033[2A"); // 向上移动2行到标题行
        
        // 重新打印标题行（蓝色背景，红色标题）
        Serial.print(STYLE_DATA_WINDOW_TITLE + "        === Encoder Diagnostics ===        " + STYLE_RESET); Serial.println();
        
        // 更新位置数据行（蓝色背景，白色正文）
        Serial.print(STYLE_DATA_WINDOW_CONTENT);
        Serial.print(title);
        Serial.print(": Position = ");
        Serial.print(position);
        Serial.println("                      " + STYLE_RESET);
    }
    
    previousEncoderPosition = position;
}

// 显示诊断值
void Terminal::displayDiagnosticValues(const String& title, const String& value1, const String& value2) {
    // 检查是否是第一次显示
    static bool firstDisplay = true;
    
    if (firstDisplay) {
        // 第一次显示时，打印两行格式（蓝色背景，红色标题，白色正文）
        Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "        === Diagnostic Values ===        " + STYLE_RESET);
        Serial.print(STYLE_DATA_WINDOW_CONTENT);
        Serial.print(title);
        Serial.print(": ");
        Serial.print(value1);
        Serial.print(", ");
        Serial.print(value2);
        Serial.println("                      " + STYLE_RESET);
        firstDisplay = false;
    } else {
        // 使用回到行首的方式更新两行数据
        Serial.print("\033[2A"); // 向上移动2行到标题行
        
        // 重新打印标题行（蓝色背景，红色标题）
        Serial.print(STYLE_DATA_WINDOW_TITLE + "        === Diagnostic Values ===        " + STYLE_RESET); Serial.println();
        
        // 更新诊断数据行（蓝色背景，白色正文）
        Serial.print(STYLE_DATA_WINDOW_CONTENT);
        Serial.print(title);
        Serial.print(": ");
        Serial.print(value1);
        Serial.print(", ");
        Serial.print(value2);
        Serial.println("                      " + STYLE_RESET);
    }
}

// 显示多行文本
void Terminal::displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3, const String& line4) {
    // 检查是否是第一次显示
    static bool firstDisplay = true;
    
    // 计算需要的行数
    int lineCount = 3 + (line3.isEmpty() ? 0 : 1) + (line4.isEmpty() ? 0 : 1);
    
    if (firstDisplay) {
        // 第一次显示时，打印多行格式（蓝色背景，红色标题，白色正文）
        Serial.println("\n" + STYLE_DATA_WINDOW_TITLE + "        === " + title + " ===        " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + line1 + "                      " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + line2 + "                      " + STYLE_RESET);
        if (!line3.isEmpty()) {
            Serial.println(STYLE_DATA_WINDOW_CONTENT + line3 + "                      " + STYLE_RESET);
        }
        if (!line4.isEmpty()) {
            Serial.println(STYLE_DATA_WINDOW_CONTENT + line4 + "                      " + STYLE_RESET);
        }
        firstDisplay = false;
    } else {
        // 使用回到行首的方式更新多行数据
        Serial.print("\033[" + String(lineCount) + "A"); // 向上移动到标题行
        
        // 重新打印标题行（蓝色背景，红色标题）
        Serial.print(STYLE_DATA_WINDOW_TITLE + "        === " + title + " ===        " + STYLE_RESET); Serial.println();
        
        // 更新数据行（蓝色背景，白色正文）
        Serial.println(STYLE_DATA_WINDOW_CONTENT + line1 + "                      " + STYLE_RESET);
        Serial.println(STYLE_DATA_WINDOW_CONTENT + line2 + "                      " + STYLE_RESET);
        if (!line3.isEmpty()) {
            Serial.println(STYLE_DATA_WINDOW_CONTENT + line3 + "                      " + STYLE_RESET);
        }
        if (!line4.isEmpty()) {
            Serial.println(STYLE_DATA_WINDOW_CONTENT + line4 + "                      " + STYLE_RESET);
        }
    }
}

// 重置诊断模式
void Terminal::resetDiagnosticMode() {
  // 目前不需要特殊操作
}

// 清理屏幕
void Terminal::clearDisplay() {
  // 串口终端可以通过发送大量换行或 ANSI 转义符来清理
  // 这里我们简单打印一个分隔符
  Serial.println("\n--- [Display Cleared] ---\n");
}

// 渲染菜单系统（串口终端不显示菜单）
void Terminal::renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) {
    // 串口暂时可留空不渲染菜单树，或后续根据需要打印
}
