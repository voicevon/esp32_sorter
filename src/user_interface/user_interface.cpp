#include "user_interface.h"
#include "main.h"
#include "oled.h"
#include "terminal.h"

// 初始化静态实例指针
UserInterface* UserInterface::instance = nullptr;

// 私有构造函数实现
UserInterface::UserInterface() {
    oled = OLED::getInstance();
    hmi = SimpleHMI::getInstance();
    terminal = Terminal::getInstance();
    
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

// 初始化所有 UI 硬件
void UserInterface::initialize() {
    hmi->initialize();
    oled->initialize();
    terminal->initialize();
}

// 更新显示内容 - 已移除，改用功能专用方法

// 显示模式变化信息
void UserInterface::displayModeChange(SystemMode newMode) {
    // 调用Terminal类的方法处理串口输出
    if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
        terminal->displayModeChange(newMode);
    }
    
    // 调用OLED类的方法处理OLED显示
    if (isOutputChannelEnabled(OUTPUT_OLED)) {
        oled->displayModeChange(newMode);
    }
}

// 显示出口状态变化
void UserInterface::displayOutletStatus(uint8_t outletIndex, bool isOpen) {
    // 输出到OLED（如果启用）
    if (isOutputChannelEnabled(OUTPUT_OLED)) {
        oled->displayOutletStatus(outletIndex, isOpen);
    }
    
    // 输出到串口（如果启用）
    if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
        terminal->displayOutletStatus(outletIndex, isOpen);
    }
}

// 显示诊断信息
void UserInterface::displayDiagnosticInfo(const String& title, const String& info) {
    // 输出到OLED（如果启用）
    if (isOutputChannelEnabled(OUTPUT_OLED)) {
        oled->displayDiagnosticInfo(title, info);
    }
    
    // 输出到串口（如果启用）
    if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
        terminal->displayDiagnosticInfo(title, info);
    }
}

// 显示出口测试模式图形
void UserInterface::displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) {
    // 输出到OLED（如果启用）
    if (isOutputChannelEnabled(OUTPUT_OLED)) {
        oled->displayOutletTestGraphic(outletCount, openOutlet, subMode);
    }
    
    // 输出到串口（如果启用）
    if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
        terminal->displayOutletTestGraphic(outletCount, openOutlet, subMode);
    }
}
// 显示扫描仪编码器值
void UserInterface::displayScannerEncoderValues(const int* risingValues, const int* fallingValues) {
    // 输出到OLED（如果启用）
    if (isOutputChannelEnabled(OUTPUT_OLED)) {
        oled->displayScannerEncoderValues(risingValues, fallingValues);
    }
    
    // 输出到串口（如果启用）
    if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
        terminal->displayScannerEncoderValues(risingValues, fallingValues);
    }
}

// 显示系统仪表盘
void UserInterface::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    // 检查是否可以更新显示
    if (isUpdateReady()) {
        // 输出到OLED（如果启用）
        if (isOutputChannelEnabled(OUTPUT_OLED)) {
            oled->displayNormalModeStats(sortingSpeedPerSecond, sortingSpeedPerMinute, sortingSpeedPerHour, identifiedCount, transportedTrayCount);
        }
        
        // 输出到串口（如果启用）
        if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
            terminal->displayDashboard(sortingSpeedPerSecond, sortingSpeedPerMinute, sortingSpeedPerHour, identifiedCount, transportedTrayCount);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

// 显示正常模式直径信息
void UserInterface::displayNormalModeDiameter(int latestDiameter) {
    // 检查是否可以更新显示
    if (isUpdateReady()) {
        // 输出到OLED（如果启用）
        if (isOutputChannelEnabled(OUTPUT_OLED)) {
            oled->displayNormalModeDiameter(latestDiameter);
        }
        
        // 输出到串口（如果启用）
        if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
            terminal->displayNormalModeDiameter(latestDiameter);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

// 通用显示方法实现
void UserInterface::displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) {
    // 检查是否可以更新显示
    if (isUpdateReady()) {
        // 输出到OLED（如果启用）
        if (isOutputChannelEnabled(OUTPUT_OLED)) {
            oled->displaySpeedStats(speedPerSecond, speedPerMinute, speedPerHour, itemCount, trayCount);
        }
        
        // 输出到串口（如果启用）
        if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
            terminal->displaySpeedStats(speedPerSecond, speedPerMinute, speedPerHour, itemCount, trayCount);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

void UserInterface::displaySingleValue(const String& label, int value, const String& unit) {
    // 检查是否可以更新显示
    if (isUpdateReady()) {
        // 输出到OLED（如果启用）
        if (isOutputChannelEnabled(OUTPUT_OLED)) {
            oled->displaySingleValue(label, value, unit);
        }
        
        // 输出到串口（如果启用）
        if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
            terminal->displaySingleValue(label, value, unit);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

void UserInterface::displayPositionInfo(const String& title, int position, bool showOnlyOnChange) {
    // 检查是否可以更新显示
    if (isUpdateReady() || showOnlyOnChange) {
        // 输出到OLED（如果启用）
        if (isOutputChannelEnabled(OUTPUT_OLED)) {
            oled->displayPositionInfo(title, position, showOnlyOnChange);
        }
        
        // 输出到串口（如果启用）
        if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
            terminal->displayPositionInfo(title, position, showOnlyOnChange);
        }
        
        // 更新上次更新时间
        updateLastUpdateTime();
    }
}

void UserInterface::displayDiagnosticValues(const String& title, const String& value1, const String& value2) {
    // 输出到OLED（如果启用）
    if (isOutputChannelEnabled(OUTPUT_OLED)) {
        oled->displayDiagnosticValues(title, value1, value2);
    }
    
    // 输出到串口（如果启用）
    if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
        terminal->displayDiagnosticValues(title, value1, value2);
    }
}

void UserInterface::displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3) {
    // 输出到OLED（如果启用）
    if (isOutputChannelEnabled(OUTPUT_OLED)) {
        oled->displayMultiLineText(title, line1, line2, line3);
    }
    
    // 输出到串口（如果启用）
    if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
        terminal->displayMultiLineText(title, line1, line2, line3);
    }
}

// 更新的模式变化显示方法
void UserInterface::displayModeChange(const String& newModeName) {
    // 输出到OLED（如果启用）
    if (isOutputChannelEnabled(OUTPUT_OLED)) {
        oled->displayModeChange(newModeName);
    }
    
    // 输出到串口（如果启用）
    if (isOutputChannelEnabled(OUTPUT_SERIAL)) {
        terminal->displayModeChange(newModeName);
    }
}

void UserInterface::resetDiagnosticMode() {
    oled->resetDiagnosticMode();
    terminal->resetDiagnosticMode();
}

// 检查显示器是否可用
bool UserInterface::isDisplayAvailable() const {
    return oled->isAvailable();
}

// 检查主按钮是否被按下
bool UserInterface::isMasterButtonPressed() {
    return hmi->isMasterButtonPressed();
}

// 检查从按钮是否被按下
bool UserInterface::isSlaveButtonPressed() {
    return hmi->isSlaveButtonPressed();
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


