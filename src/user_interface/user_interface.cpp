#include "user_interface.h"
#include "main.h"

// 初始化静态实例指针
UserInterface* UserInterface::instance = nullptr;

// 私有构造函数实现
UserInterface::UserInterface() {
    hmi = SimpleHMI::getInstance();
    
    // 初始化显示设备数组
    displayDeviceCount = 0;
    for (int i = 0; i < MAX_DISPLAY_DEVICES; i++) {
        displayDevices[i] = nullptr;
    }
    
    // 初始化输出渠道（默认禁用所有输出渠道，需要在setup()中显式启用）
    outputChannels = 0;
    
    // 初始化语言（默认英语）
    currentLanguage = LANGUAGE_ENGLISH;
    
    // 初始化上次更新时间
    lastUpdateTime = 0;
}

// 检查是否可以更新显示
bool UserInterface::isUpdateReady() const {
    return millis() - lastUpdateTime >= UPDATE_INTERVAL;
}

// 更新上次更新时间
void UserInterface::updateLastUpdateTime() {
    lastUpdateTime = millis();
}

// 获取单例实例
UserInterface* UserInterface::getInstance() {
    if (instance == nullptr) {
        instance = new UserInterface();
    }
    return instance;
}

// 初始化所有 UI 硬件（不创建显示设备实例）
void UserInterface::initialize() {
    hmi->initialize();
    // 不再创建显示设备实例，改为由外部注入
}

// 添加显示设备的静态方法（允许外部注入显示设备）
void UserInterface::addExternalDisplayDevice(Display* display) {
    if (display != nullptr) {
        // 获取UserInterface实例
        UserInterface* ui = getInstance();
        // 将显示设备添加到数组中
        ui->addDisplayDevice(display);
    }
}

// 更新显示内容 - 已移除，改用功能专用方法

// 显示模式变化信息
void UserInterface::displayModeChange(SystemMode newMode) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->displayModeChange(newMode);
    }
}

// 显示出口状态变化
void UserInterface::displayOutletStatus(uint8_t outletIndex, bool isOpen) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->displayOutletStatus(outletIndex, isOpen);
    }
}

// 显示诊断信息
void UserInterface::displayDiagnosticInfo(const String& title, const String& info) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->displayDiagnosticInfo(title, info);
    }
}

// 显示出口测试模式图形
void UserInterface::displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->displayOutletTestGraphic(outletCount, openOutlet, subMode);
    }
}

// 专门用于寿命测试的显示方法
void UserInterface::displayOutletTestGraphic(uint8_t outletCount, unsigned long cycleCount, bool outletState, int subMode) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        // 调用寿命测试专用的显示方法，直接传递完整的循环次数
        displayDevices[i]->displayOutletLifetimeTestGraphic(outletCount, cycleCount, outletState, subMode);
    }
}

// 显示扫描仪编码器值
void UserInterface::displayScannerEncoderValues(const int* risingValues, const int* fallingValues) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->displayScannerEncoderValues(risingValues, fallingValues);
    }
}

// 显示系统仪表盘
void UserInterface::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    // 检查是否可以更新显示
    if (isUpdateReady()) {
        // 遍历所有显示设备
        for (int i = 0; i < displayDeviceCount; i++) {
            displayDevices[i]->displayDashboard(sortingSpeedPerSecond, sortingSpeedPerMinute, sortingSpeedPerHour, identifiedCount, transportedTrayCount);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

// 显示直径信息（功能专用方法）
void UserInterface::displayDiameter(int latestDiameter) {
    // 检查是否可以更新显示
    if (isUpdateReady()) {
        // 遍历所有显示设备
        for (int i = 0; i < displayDeviceCount; i++) {
            // 调用显示设备的displayNormalModeDiameter方法（兼容接口）
            displayDevices[i]->displayNormalModeDiameter(latestDiameter);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

// 显示正常模式直径信息（兼容旧接口）
void UserInterface::displayNormalModeDiameter(int latestDiameter) {
    // 调用新的功能专用方法
    displayDiameter(latestDiameter);
}

// 通用显示方法实现
void UserInterface::displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) {
    // 检查是否可以更新显示
    if (isUpdateReady()) {
        // 遍历所有显示设备
        for (int i = 0; i < displayDeviceCount; i++) {
            displayDevices[i]->displaySpeedStats(speedPerSecond, speedPerMinute, speedPerHour, itemCount, trayCount);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

void UserInterface::displaySingleValue(const String& label, int value, const String& unit) {
    // 检查是否可以更新显示
    if (isUpdateReady()) {
        // 遍历所有显示设备
        for (int i = 0; i < displayDeviceCount; i++) {
            displayDevices[i]->displaySingleValue(label, value, unit);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

void UserInterface::displayPositionInfo(const String& title, int position, bool showOnlyOnChange) {
    // 检查是否可以更新显示
    if (isUpdateReady() || showOnlyOnChange) {
        // 遍历所有显示设备
        for (int i = 0; i < displayDeviceCount; i++) {
            displayDevices[i]->displayPositionInfo(title, position, showOnlyOnChange);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

void UserInterface::displayDiagnosticValues(const String& title, const String& value1, const String& value2) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->displayDiagnosticValues(title, value1, value2);
    }
}

void UserInterface::displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3, const String& line4) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->displayMultiLineText(title, line1, line2, line3, line4);
    }
}

// 更新的模式变化显示方法
void UserInterface::displayModeChange(const String& newModeName) {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->displayModeChange(newModeName);
    }
}

void UserInterface::resetDiagnosticMode() {
    // 遍历所有显示设备
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->resetDiagnosticMode();
    }
}

// 检查显示器是否可用
bool UserInterface::isDisplayAvailable() const {
    // 如果有任何显示设备可用，则返回true
    for (int i = 0; i < displayDeviceCount; i++) {
        if (displayDevices[i]->isAvailable()) {
            return true;
        }
    }
    return false;
}

// 检查主按钮是否被按下
bool UserInterface::isMasterButtonPressed() {
    return hmi->isMasterButtonPressed();
}

// 检查从按钮是否被按下
bool UserInterface::isSlaveButtonPressed() {
    return hmi->isSlaveButtonPressed();
}

// 检查主按钮是否被长按
bool UserInterface::isMasterButtonLongPressed() {
    return hmi->isMasterButtonLongPressed();
}

// 检查从按钮是否被长按
bool UserInterface::isSlaveButtonLongPressed() {
    return hmi->isSlaveButtonLongPressed();
}

// 启用指定输出渠道
void UserInterface::enableOutputChannel(OutputChannel channel) {
    outputChannels |= channel;
}

// 禁用指定输出渠道
void UserInterface::disableOutputChannel(OutputChannel channel) {
    outputChannels &= ~channel;
}

// 检查输出渠道是否启用
bool UserInterface::isOutputChannelEnabled(OutputChannel channel) const {
    return (outputChannels & channel) != 0;
}

// 设置所有输出渠道
void UserInterface::setOutputChannels(uint8_t channels) {
    outputChannels = channels;
}

// 获取当前输出渠道设置
uint8_t UserInterface::getOutputChannels() const {
    return outputChannels;
}

// 添加显示设备
bool UserInterface::addDisplayDevice(Display* display) {
    if (display == nullptr || displayDeviceCount >= MAX_DISPLAY_DEVICES) {
        return false;
    }
    
    // 检查设备是否已经存在
    for (int i = 0; i < displayDeviceCount; i++) {
        if (displayDevices[i] == display) {
            return false;
        }
    }
    
    // 添加新设备
    displayDevices[displayDeviceCount++] = display;
    return true;
}

// 移除显示设备
bool UserInterface::removeDisplayDevice(Display* display) {
    if (display == nullptr) {
        return false;
    }
    
    // 查找设备
    int index = -1;
    for (int i = 0; i < displayDeviceCount; i++) {
        if (displayDevices[i] == display) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        return false;
    }
    
    // 移除设备并重新排列数组
    for (int i = index; i < displayDeviceCount - 1; i++) {
        displayDevices[i] = displayDevices[i + 1];
    }
    displayDevices[--displayDeviceCount] = nullptr;
    return true;
}

// 清除所有显示设备
void UserInterface::clearAllDisplayDevices() {
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i] = nullptr;
    }
    displayDeviceCount = 0;
}

// 设置当前语言
void UserInterface::setLanguage(Language language) {
    currentLanguage = language;
}

// 获取当前语言
Language UserInterface::getLanguage() const {
    return currentLanguage;
}

// 翻译方法（根据当前语言翻译文本）
String UserInterface::translate(const String& key) const {
    // 简单的翻译映射示例，可以扩展为更复杂的翻译系统
    if (currentLanguage == LANGUAGE_CHINESE) {
        if (key == "System ready") return "系统就绪";
        if (key == "Current Mode") return "当前模式";
        if (key == "Use mode button to switch between modes") return "使用模式按钮切换模式";
        if (key == "Mode switch requested to") return "请求切换模式到";
        if (key == "Mode switched to") return "模式已切换到";
        if (key == "Normal Mode") return "正常模式";
        if (key == "Encoder Diag") return "编码器诊断";
        if (key == "Scanner Diag") return "扫描仪诊断";
        if (key == "Outlet Diag") return "出口诊断";
        if (key == "Feeder Test") return "上料器测试";
        if (key == "Version Info") return "版本信息";
        if (key == "Submode") return "子模式";
        if (key == "Switch to Submode") return "切换到子模式";
        if (key == "Stats") return "统计信息";
        if (key == "Latest Diameter") return "最新直径";
        if (key == "Phase Change") return "相位变化";
        if (key == "Position") return "位置";
        if (key == "Cycle Drop (Normally Open)") return "循环降落（常开）";
        if (key == "Cycle Raise (Normally Closed)") return "循环上升（常闭）";
        if (key == "Encoder Diagnostic Mode Activated") return "编码器诊断模式已激活";
        if (key == "Use slave button to switch submode") return "使用从按钮切换子模式";
        if (key == "Outlet Diagnostic Mode Activated") return "出口诊断模式已激活";
        if (key == "Feeder Test Mode Activated") return "上料器测试模式已激活";
    }
    // 默认返回原始字符串
    return key;
}

// 输出消息到指定渠道
void UserInterface::output(const String& message, OutputChannel channels) {
    // 翻译消息
    String translatedMessage = translate(message);
    
    // 输出到串口
    if ((channels & OUTPUT_SERIAL) && isOutputChannelEnabled(OUTPUT_SERIAL)) {
        Serial.print(translatedMessage);
    }
    
    // OLED输出需要特殊处理，暂时只支持串口
    // 可以在未来扩展OLED的直接文本输出功能
}

// 输出带换行的消息到指定渠道
void UserInterface::outputLine(const String& message, OutputChannel channels) {
    // 翻译消息
    String translatedMessage = translate(message);
    
    // 输出到串口
    if ((channels & OUTPUT_SERIAL) && isOutputChannelEnabled(OUTPUT_SERIAL)) {
        Serial.println(translatedMessage);
    }
    
    // OLED输出需要特殊处理，暂时只支持串口
    // 可以在未来扩展OLED的直接文本输出功能
}


