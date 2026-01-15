// 精简版人机交互模块
#ifndef SIMPLE_HMI_H
#define SIMPLE_HMI_H

#include <Arduino.h>
#include "pins.h"

// 防抖动参数
#define DEBOUNCE_DELAY 50  // 防抖动延迟时间（毫秒）

// 基本按钮和LED功能定义 - 单例模式实现
class SimpleHMI {
private:
    // 引脚配置
    int masterButtonPin;
    int slaveButtonPin;
    int masterLEDPin;
    int slaveLEDPin;
    
    // 中断相关变量
    volatile bool masterButtonFlag;        // 最终的按钮按下标志（按下并释放）
    volatile bool slaveButtonFlag;         // 最终的按钮按下标志（按下并释放）
    volatile bool masterButtonPressed;     // 临时的按钮按下状态
    volatile bool slaveButtonPressed;      // 临时的按钮按下状态
    unsigned long lastMasterDebounceTime;
    unsigned long lastSlaveDebounceTime;
    
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
    
    // 清除按钮标志（手动）

    

    
    // 中断处理函数需要访问私有成员
    friend void IRAM_ATTR masterButtonISR();
    friend void IRAM_ATTR slaveButtonISR();
    
private:
    // 静态实例指针
    static SimpleHMI* instance;
};

#endif // SIMPLE_HMI_H