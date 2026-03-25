#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <Arduino.h>
#include "user_interface/simple_hmi.h"
#include "user_interface/display.h"  // 包含Display抽象基类

// 前向声明，不需要包含具体实现的头文件
class OLED;
class Terminal;
// #include "display_data.h"已移除，不再需要

// 系统工作模式前向声明
enum SystemMode;

// 输出渠道枚举
enum OutputChannel {
    OUTPUT_SERIAL = 1 << 0,   // 串口输出
    OUTPUT_OLED = 1 << 1,     // OLED显示
    OUTPUT_ALL = OUTPUT_SERIAL | OUTPUT_OLED  // 所有输出渠道
};

// 语言枚举
enum Language {
    LANGUAGE_ENGLISH,
    LANGUAGE_CHINESE
};

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
    SimpleHMI* hmi;
    
    // 显示设备数组
    static const int MAX_DISPLAY_DEVICES = 4;  // 最大显示设备数量
    Display* displayDevices[MAX_DISPLAY_DEVICES];  // 显示设备数组
    int displayDeviceCount;  // 当前显示设备数量
    
    // 输出控制
    uint8_t outputChannels;  // 输出渠道掩码
    Language currentLanguage;  // 当前语言
    uint32_t lastUpdateTime;  // 上次更新时间（用于限制刷新速率）
    const uint32_t UPDATE_INTERVAL = 2000;  // 更新间隔（毫秒）
    
    // 辅助方法
    String translate(const String& key) const;  // 根据当前语言翻译文本
    bool isUpdateReady() const;  // 检查是否可以更新显示
    void updateLastUpdateTime();  // 更新上次更新时间

public:
    // 获取单例实例（静态方法）
    static UserInterface* getInstance();
    
    // 初始化所有 UI 硬件（不创建显示设备实例）
  void initialize();
  
  // 添加显示设备的静态方法（允许外部注入显示设备）
  static void addExternalDisplayDevice(Display* display);
    
    // 显示相关方法 - updateDisplay已移除，改用功能专用方法
    void displayModeChange(SystemMode newMode);
    void displayOutletStatus(uint8_t outletIndex, bool isOpen);
    void displayDiagnosticInfo(const String& title, const String& info);
    
    // 显示配置详情（带针对长度选择的反色显示）
    void displayConfigEdit(const String& title, int maxV, int minV, uint8_t targetMode, int activeField);

    // 显示出口测试模式图形
    void displayOutletTestGraphic(uint8_t outletCount, uint8_t selectedOutlet, bool isOpen, int subMode);
    
    // 专门用于寿命测试的显示方法 (更名以避免重载歧义)
    void displayOutletLifetimeGraphic(uint8_t outletCount, uint32_t cycleCount, bool outletState, int subMode);
    void displayScannerEncoderValues(const int* risingValues, const int* fallingValues);
    
    // 显示系统仪表盘
    void displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount, int latestDiameter, int latestScanCount, int latestLengthLevel = 0, bool forceRefresh = false);
    void displayNormalModeDiameter(int latestDiameter, bool forceRefresh = false);
    
    // 统一菜单显示代理
    void renderMenu(MenuNode* node, int cursorIndex, int scrollOffset);
    
    // 通用显示方法（替代旧的updateDisplay）
  void displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount);
  void displayDiameter(int latestDiameter, bool forceRefresh = false);
  void displaySingleValue(const String& label, int value, const String& unit);
  void displayPositionInfo(const String& title, int position, bool showOnlyOnChange);
  void displayDiagnosticValues(const String& title, const String& value1, const String& value2);
  void displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3 = "", const String& line4 = "", const String& line5 = "");
  

    
    // 更新的模式变化显示方法
    void displayModeChange(const String& newModeName);
    
    void resetDiagnosticMode();
    bool isDisplayAvailable() const;
    void clearDisplay(); // 新增：清理所有显示设备的屏幕
    
    // 输入相关方法
    // 获取编码器逻辑旋转增量 (通常带分频)
    int getEncoderDelta();
    
    // 获取编码器原始物理增量 (1:1，无分频，高灵敏度)
    int getRawEncoderDelta();
    bool isMasterButtonPressed();
    bool isMasterButtonLongPressed();
    
    // 显示设备管理方法
    bool addDisplayDevice(Display* display);  // 添加显示设备
    bool removeDisplayDevice(Display* display);  // 移除显示设备
    void clearAllDisplayDevices();  // 清除所有显示设备
    
    // 输出控制方法
    void enableOutputChannel(OutputChannel channel);  // 启用指定输出渠道
    void disableOutputChannel(OutputChannel channel);  // 禁用指定输出渠道
    bool isOutputChannelEnabled(OutputChannel channel) const;  // 检查输出渠道是否启用
    void setOutputChannels(uint8_t channels);  // 设置所有输出渠道
    uint8_t getOutputChannels() const;  // 获取当前输出渠道设置
    
    // 语言控制方法
    void setLanguage(Language language);  // 设置当前语言
    Language getLanguage() const;  // 获取当前语言
    
    // 统一输出方法
    void output(const String& message, OutputChannel channels = OUTPUT_ALL);  // 输出消息到指定渠道
    void outputLine(const String& message, OutputChannel channels = OUTPUT_ALL);  // 输出带换行的消息到指定渠道

};

#endif // USER_INTERFACE_H
