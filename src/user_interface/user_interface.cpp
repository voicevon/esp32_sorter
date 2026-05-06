#include "user_interface.h"
#include "main.h"

// 初始化静态实例指针
UserInterface* UserInterface::instance = nullptr;

// 私有构造函数实现
UserInterface::UserInterface() {
    instance = this; // 立即赋值，防止 addInputSource 内部触发递归
    
    // 初始化显示设备数组
    displayDeviceCount = 0;
    for (int i = 0; i < MAX_DISPLAY_DEVICES; i++) {
        displayDevices[i] = nullptr;
    }
    
    // 初始化输入设备数组
    inputSourceCount = 0;
    for (int i = 0; i < MAX_INPUT_SOURCES; i++) {
        inputSources[i] = nullptr;
    }
    
    // 不再默认添加物理旋钮，改为在 main.cpp 中显式添加
    
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

// 初始化所有 UI 状态
void UserInterface::initialize() {
    // 不再初始化硬件，硬件初始化移至 main.cpp
    // 不再创建显示设备实例，改为由外部注入
}

// 添加显示设备的静态方法（允许外部注入显示设备）
void UserInterface::addExternalDisplayDevice(Display* display) {
    // 获取UserInterface实例，将显示设备添加到数组中
    getInstance()->addDisplayDevice(display);
}

// 添加输入源的静态方法
void UserInterface::addInputSource(InputSource* source) {
    if (source == nullptr) return;
    
    UserInterface* ui = getInstance();
    if (ui->inputSourceCount >= MAX_INPUT_SOURCES) return;
    
    // 检查重复
    for (int i = 0; i < ui->inputSourceCount; i++) {
        if (ui->inputSources[i] == source) return;
    }
    
    ui->inputSources[ui->inputSourceCount++] = source;
}

void UserInterface::refreshAllDevices(const DisplaySnapshot& snapshot) {
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->refresh(snapshot);
    }
}

void UserInterface::renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) {
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->renderMenu(node, cursorIndex, scrollOffset);
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

// 清理所有显示设备的屏幕
void UserInterface::clearDisplay() {
    for (int i = 0; i < displayDeviceCount; i++) {
        displayDevices[i]->clearDisplay();
    }
}

// 获取下一个待处理的用户意图
UIIntent UserInterface::getNextIntent() {
    // 依次轮询所有注册的输入源
    for (int i = 0; i < inputSourceCount; i++) {
        if (inputSources[i] == nullptr) continue;
        
        // 执行驱动心跳（如处理异步读取、消抖等）
        inputSources[i]->tick();
        
        // 检查并消费意图
        if (inputSources[i]->hasIntent()) {
            return inputSources[i]->pollIntent();
        }
    }
    
    return UIIntent(UIAction::NONE);
}

// [已弃用] 获取编码器旋转增量
int UserInterface::getEncoderDelta() {
    return 0;
}
 
// [已弃用] 获取原始旋转增量 (1:1 高灵敏度)
int UserInterface::getRawEncoderDelta() {
    return 0;
}
 
// [已弃用] 检查主按钮是否被按下
bool UserInterface::isMasterButtonPressed() {
    return false;
}
 
// [已弃用] 检查主按钮是否被长按
bool UserInterface::isMasterButtonLongPressed() {
    return false;
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

// 清除所有输入源
void UserInterface::clearAllInputSources() {
    for (int i = 0; i < inputSourceCount; i++) {
        inputSources[i] = nullptr;
    }
    inputSourceCount = 0;
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


