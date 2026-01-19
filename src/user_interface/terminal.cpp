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
    lastUpdateTime = 0;
    
    // 初始化上一次显示的数据
    lastDisplayedMode = MODE_NORMAL;
    lastEncoderPosition = 0;
    lastSortingSpeedPerSecond = 0.0;
    lastSortingSpeedPerMinute = 0;
    lastSortingSpeedPerHour = 0;
    lastIdentifiedCount = 0;
    lastTransportedTrayCount = 0;
    lastLatestDiameter = 0;
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
    return millis() - lastUpdateTime >= UPDATE_INTERVAL;
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
        case MODE_TEST_RELOADER:
            modeName = "Feeder Test";
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
    Serial.print("Mode changed to: ");
    Serial.println(newModeName);
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
    Serial.print(title);
    Serial.print(": ");
    Serial.println(info);
}

// 显示出口测试模式图形
void Terminal::displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) {
    String subModeName = (subMode == 0) ? "Cycle Drop" : "Cycle Raise";
    
    Serial.println("Outlet Test:");
    Serial.print("  Outlet Count: ");
    Serial.println(outletCount);
    Serial.print("  Open Outlet: ");
    Serial.println(openOutlet);
    Serial.print("  Submode: ");
    Serial.println(subModeName);
}

// 显示扫描仪编码器值
void Terminal::displayScannerEncoderValues(const int* risingValues, const int* fallingValues) {
    Serial.println("Scanner Encoder Values:");
    Serial.print("  Rising: ");
    for (int i = 0; i < 8; i++) {
        Serial.print(risingValues[i]);
        Serial.print(" ");
    }
    Serial.println();
    
    Serial.print("  Falling: ");
    for (int i = 0; i < 8; i++) {
        Serial.print(fallingValues[i]);
        Serial.print(" ");
    }
    Serial.println();
}

// 显示系统仪表盘
void Terminal::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    if (isUpdateReady() || 
        sortingSpeedPerSecond != lastSortingSpeedPerSecond ||
        sortingSpeedPerMinute != lastSortingSpeedPerMinute ||
        sortingSpeedPerHour != lastSortingSpeedPerHour ||
        identifiedCount != lastIdentifiedCount ||
        transportedTrayCount != lastTransportedTrayCount) {
        
        Serial.println("System Dashboard:");
        Serial.print("  Speed: ");
        Serial.print(sortingSpeedPerSecond, 1);
        Serial.print("/s, ");
        Serial.print(sortingSpeedPerMinute);
        Serial.print("/min, ");
        Serial.print(sortingSpeedPerHour);
        Serial.println("/h");
        
        Serial.print("  Identified Items: ");
        Serial.println(identifiedCount);
        
        Serial.print("  Transported Trays: ");
        Serial.println(transportedTrayCount);
        
        // 更新上次显示的数据
        lastSortingSpeedPerSecond = sortingSpeedPerSecond;
        lastSortingSpeedPerMinute = sortingSpeedPerMinute;
        lastSortingSpeedPerHour = sortingSpeedPerHour;
        lastIdentifiedCount = identifiedCount;
        lastTransportedTrayCount = transportedTrayCount;
        
        // 更新上次更新时间
        lastUpdateTime = millis();
    }
}

// 显示正常模式直径信息
void Terminal::displayNormalModeDiameter(int latestDiameter) {
    if (isUpdateReady() || latestDiameter != lastLatestDiameter) {
        Serial.print("Latest Diameter: ");
        Serial.print(latestDiameter);
        Serial.println(" mm");
        
        lastLatestDiameter = latestDiameter;
        lastUpdateTime = millis();
    }
}

// 显示正常模式统计信息（保持兼容）
void Terminal::displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    displayDashboard(sortingSpeedPerSecond, sortingSpeedPerMinute, sortingSpeedPerHour, identifiedCount, transportedTrayCount);
}

// 显示速度统计信息
void Terminal::displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) {
    if (isUpdateReady()) {
        Serial.println("Speed Statistics:");
        Serial.print("  Speed: ");
        Serial.print(speedPerSecond);
        Serial.print("/s, ");
        Serial.print(speedPerMinute);
        Serial.print("/min, ");
        Serial.print(speedPerHour);
        Serial.println("/h");
        
        Serial.print("  Items Processed: ");
        Serial.println(itemCount);
        
        Serial.print("  Trays Transported: ");
        Serial.println(trayCount);
        
        lastUpdateTime = millis();
    }
}

// 显示单个值
void Terminal::displaySingleValue(const String& label, int value, const String& unit) {
    Serial.print(label);
    Serial.print(": ");
    Serial.print(value);
    if (!unit.isEmpty()) {
        Serial.print(" ");
        Serial.print(unit);
    }
    Serial.println();
}

// 显示位置信息
void Terminal::displayPositionInfo(const String& title, int position, bool showOnlyOnChange) {
    if (showOnlyOnChange && position == lastEncoderPosition) {
        return;
    }
    
    Serial.print(title);
    Serial.print(": Position = ");
    Serial.println(position);
    
    lastEncoderPosition = position;
}

// 显示诊断值
void Terminal::displayDiagnosticValues(const String& title, const String& value1, const String& value2) {
    Serial.print(title);
    Serial.print(": ");
    Serial.print(value1);
    Serial.print(", ");
    Serial.println(value2);
}

// 显示多行文本
void Terminal::displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3) {
    Serial.println(title);
    Serial.println(line1);
    Serial.println(line2);
    if (!line3.isEmpty()) {
        Serial.println(line3);
    }
}

// 重置诊断模式
void Terminal::resetDiagnosticMode() {
    Serial.println("Diagnostic mode reset");
}
