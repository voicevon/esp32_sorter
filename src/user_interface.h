#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <Arduino.h>
#include "oled.h"
#include "simple_hmi.h"
#include "display_data.h"

// 系统工作模式前向声明
enum SystemMode;

/**
 * @class UserInterface
 * @brief 用户界面管理类，合并 OLED 显示和 SimpleHMI 输入功能
 * 
 * 该类使用单例模式实现，负责管理所有用户交互功能
 * 包括 OLED 显示、按钮输入、LED 输出等
 */
class UserInterface {
private:
    // 私有构造函数（单例模式）
    UserInterface();
    
    // 防止拷贝
    UserInterface(const UserInterface&) = delete;
    UserInterface& operator=(const UserInterface&) = delete;
    
    // 静态实例指针
    static UserInterface* instance;
    
    // 内部组件
    OLED* oled;
    SimpleHMI* hmi;

public:
    // 获取单例实例（静态方法）
    static UserInterface* getInstance();
    
    // 初始化所有 UI 硬件
    void initialize();
    
    // 显示相关方法
    void updateDisplay(const DisplayData& data);
    void displayModeChange(SystemMode newMode);
    void displayOutletStatus(uint8_t outletIndex, bool isOpen);
    void displayDiagnosticInfo(const String& title, const String& info);
    void displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode);
    void displayScannerEncoderValues(const int* risingValues, const int* fallingValues);
    void resetDiagnosticMode();
    bool isDisplayAvailable() const;
    
    // 输入相关方法
    bool isMasterButtonPressed();
    bool isSlaveButtonPressed();

};

#endif // USER_INTERFACE_H
