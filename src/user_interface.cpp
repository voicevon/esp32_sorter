#include "user_interface.h"
#include "main.h"

// 初始化静态实例指针
UserInterface* UserInterface::instance = nullptr;

// 私有构造函数实现
UserInterface::UserInterface() {
    oled = OLED::getInstance();
    hmi = SimpleHMI::getInstance();
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
}

// 更新显示内容
void UserInterface::updateDisplay(const DisplayData& data) {
    oled->update(data);
}

// 显示模式变化信息
void UserInterface::displayModeChange(SystemMode newMode) {
    oled->displayModeChange(newMode);
}

// 显示出口状态变化
void UserInterface::displayOutletStatus(uint8_t outletIndex, bool isOpen) {
    oled->displayOutletStatus(outletIndex, isOpen);
}

// 显示诊断信息
void UserInterface::displayDiagnosticInfo(const String& title, const String& info) {
    oled->displayDiagnosticInfo(title, info);
}

// 显示出口测试模式图形
void UserInterface::displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) {
    oled->displayOutletTestGraphic(outletCount, openOutlet, subMode);
}

void UserInterface::displayScannerEncoderValues(const int* risingValues, const int* fallingValues) {
    oled->displayScannerEncoderValues(risingValues, fallingValues);
}

void UserInterface::resetDiagnosticMode() {
    oled->resetDiagnosticMode();
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


