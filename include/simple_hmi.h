// 人机交互模块定义文件
#ifndef SIMPLE_HMI_H
#define SIMPLE_HMI_H

#include <Arduino.h>
#include "pins.h"

// 按钮枚举定义
enum Button {
  MASTER_BUTTON = 1,         // 主按钮
  SLAVE_BUTTON = 2           // 从按钮
};

// LED枚举定义
enum LED {
  MASTER_LED = 1,            // 主LED
  SLAVE_LED = 2              // 从LED
};

class SimpleHMI {
private:
    // 硬件配置
    int masterButtonPin;      // 主按钮引脚
    int slaveButtonPin;       // 从按钮引脚
    int masterLEDPin;         // 主LED引脚
    int slaveLEDPin;          // 从LED引脚
    
    // 状态变量
    bool masterButtonPressed; // 主按钮按下标志
    bool slaveButtonPressed;  // 从按钮按下标志
    bool masterLEDState;      // 主LED当前状态
    bool slaveLEDState;       // 从LED当前状态
    unsigned long lastMasterButtonTime; // 主按钮最后操作时间戳
    unsigned long lastSlaveButtonTime;  // 从按钮最后操作时间戳
    
    // 配置参数
    const int DEBOUNCE_TIME = 50;  // 固定去抖时间(毫秒)
    
    // 静态成员用于中断处理
    static SimpleHMI* instance;
    
    // 中断处理函数
    static void handleMasterButtonInterrupt();
    static void handleSlaveButtonInterrupt();
    
public:
    // 构造函数
    SimpleHMI();
    
    // 初始化人机交互模块
    void initialize();
    
    // 主循环更新函数
    void spin_once();
    
    // LED控制方法
    void setLEDState(LED led, bool state);
    
    // 按钮状态检查
    bool isButtonPressed(Button button);
    void clearButtonStates();
};

#endif // SIMPLE_HMI_H