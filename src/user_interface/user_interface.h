#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <Arduino.h>
#include "common/input_source.h"
#include "common/display.h"  // 包含Display抽象基类
#include "common/display_types.h"


// 前向声明，不需要包含具体实现的头文件
class OLED;
class Terminal;
class MenuNode;
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
 * @brief 用户界面管理类，合并 OLED 显示和 RotaryInputSource 输入功能
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
    
    // 内部组件已移除，改为由外部注入
    
    // 输入设备数组
    static const int MAX_INPUT_SOURCES = 4;
    InputSource* inputSources[MAX_INPUT_SOURCES];
    int inputSourceCount;
    
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
  
  // 添加输入源的静态方法
  static void addInputSource(InputSource* source);
    
    // 核心快照刷新接口：广播至所有注册的显示器
    void refreshAllDevices(const DisplaySnapshot& snapshot);
    
    // 统一菜单显示代理
    void renderMenu(MenuNode* node, int cursorIndex, int scrollOffset);
    
    void clearDisplay(); // 新增：清理所有显示设备的屏幕
    bool isDisplayAvailable() const; // 新增：检查显示器是否可用

    
    // 意图驱动的输入方法
    UIIntent getNextIntent();
    
    // 输入相关方法 (旧接口，兼容物理旋钮)
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
    void clearAllInputSources();    // 清除所有输入源
    
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
