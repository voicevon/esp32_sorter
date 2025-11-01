// 调试模块定义文件
#ifndef DEBUG_MODULE_H
#define DEBUG_MODULE_H

#include <Arduino.h>
#include "pins.h"

// 系统工作模式定义
enum SystemMode {
  MODE_NORMAL = 0,            // 正常工作模式
  MODE_DIAGNOSE_ENCODER = 1,  // 诊断编码器模式
  MODE_DIAGNOSE_SCANNER = 2,  // 诊断扫描仪模式
  MODE_DIAGNOSE_DIVERTER = 3, // 诊断分支器模式
  MODE_DIAGNOSE_CONVEYOR = 4, // 诊断传输线模式
  MODE_TEST = 5               // 测试模式
};

class DebugModule {
private:
    // 硬件配置
    int button1Pin;
    int button2Pin;
    int led1Pin;
    int led2Pin;
    
    // 状态变量
    bool button1Pressed;
    bool button2Pressed;
    unsigned long lastButton1Time;
    unsigned long lastButton2Time;
    unsigned long lastBlinkTime;
    bool blinkState;
    bool led1State;
    bool led2State;
    int currentSystemMode;
    
    // 配置参数
    int debounceTime;
    int blinkInterval;
    
    // 静态成员用于中断处理
    static DebugModule* instance;
    
    // 中断处理函数
    static void handleButton1Interrupt();
    static void handleButton2Interrupt();
    
public:
    // 构造函数
    DebugModule();
    
    // 初始化调试模块
    void initialize();
    
    // 更新模块状态
    void update();
    
    // LED控制方法
    void setLEDState(int ledNumber, bool state);
    void toggleLED(int ledNumber);

    
    // 按钮状态检查
    bool isButtonPressed(int buttonNumber);
    void clearButtonStates();
    
    // 系统模式设置
    void setSystemMode(int mode);
    
    // 配置方法
    void setDebounceTime(int debounceMs);
    void setBlinkInterval(int intervalMs);
};

#endif // DEBUG_MODULE_H