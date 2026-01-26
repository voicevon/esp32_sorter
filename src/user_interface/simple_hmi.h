// 精简版人机交互模块
#ifndef SIMPLE_HMI_H
#define SIMPLE_HMI_H

#include <Arduino.h>
#include "modular/pins.h"

// 防抖动参数
#define DEBOUNCE_DELAY 50  // 防抖动延迟时间（毫秒）
#define LONG_PRESS_DELAY 1000  // 长按检测延迟时间（毫秒）

// 基本按钮和LED功能定义 - 单例模式实现
class SimpleHMI {
private:
    // 引脚配置
    int masterButtonPin;
    int slaveButtonPin;
    
    // 中断相关变量
    volatile bool masterButtonClickFlag;   // 最终的按钮点击标志（按下并释放）
    volatile bool slaveButtonClickFlag;    // 最终的按钮点击标志（按下并释放）
    volatile bool masterButtonLongPressFlag;   // 长按标志（按下并释放，且时间超过阈值）
    volatile bool slaveButtonLongPressFlag;    // 长按标志（按下并释放，且时间超过阈值）
    volatile bool masterButtonDownState;   // 临时的按钮按下状态
    volatile bool slaveButtonDownState;    // 临时的按钮按下状态
    unsigned long lastMasterDebounceTime;
    unsigned long lastSlaveDebounceTime;
    unsigned long masterButtonPressStartTime;
    unsigned long slaveButtonPressStartTime;
    
    // 私有构造函数，防止外部创建实例
    SimpleHMI();
    
    // 防止拷贝
    SimpleHMI(const SimpleHMI&) = delete;
    SimpleHMI& operator=(const SimpleHMI&) = delete;
    
public:
    // 获取单例实例（静态方法）
    static SimpleHMI* getInstance();
    
    // 初始化HMI
    void initialize();
    
    // 检查按钮状态（中断标志）
    // 返回true表示按钮被按下并释放了一次
    // 注意：此方法会自动清除标志
    bool isMasterButtonPressed();
    bool isSlaveButtonPressed();
    
    // 检查按钮长按状态
    // 返回true表示按钮被长按
    // 注意：此方法会自动清除标志
    bool isMasterButtonLongPressed();
    bool isSlaveButtonLongPressed();
    
    // 清除按钮标志（手动）



    // 中断处理函数需要访问私有成员
    friend void IRAM_ATTR masterButtonISR();
    friend void IRAM_ATTR slaveButtonISR();
    
private:
    // 静态实例指针
    static SimpleHMI* instance;
};

#endif // SIMPLE_HMI_H