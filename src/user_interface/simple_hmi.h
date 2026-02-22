// 精简版人机交互模块
#ifndef SIMPLE_HMI_H
#define SIMPLE_HMI_H

#include <Arduino.h>
#include "../config.h"

// 防抖动参数
#define DEBOUNCE_DELAY 50  // 防抖动延迟时间（毫秒）
#define LONG_PRESS_DELAY 1000  // 长按检测延迟时间（毫秒）
#define ENCODER_LOCKOUT 20 // 编码器计数锁定时间（毫秒），防止抖动导致双倍计数

// 基本按钮和LED功能定义 - 单例模式实现
class SimpleHMI {
private:
    // 引脚配置
    int masterButtonPin;
    int encoderPinA;
    int encoderPinB;
    
    // 中断相关变量 (Button)
    volatile bool masterButtonClickFlag;   // 最终的按钮点击标志（按下并释放）
    volatile bool masterButtonLongPressFlag;   // 长按标志（按下并释放，且时间超过阈值）
    volatile bool masterButtonDownState;   // 临时的按钮按下状态
    volatile unsigned long lastMasterDebounceTime;
    volatile unsigned long masterButtonPressStartTime;

    // 中断相关变量 (Encoder)
    volatile int lastEncoderLevelA;
    // 累积旋转增量：记录自上次读取以来的总步数。
    // 可能值：0(无位移), ±1(正常步进), ±N(极速旋转累加)。
    // 调用 getEncoderDelta() 后会自动清零，确保每次获取的都是相对位移。
    volatile int encoderDelta;
    volatile int encoderState;           // 新增：记录编码器当前状态逻辑值 (0-3)
    volatile unsigned long lastEncoderInterruptTime; // 新增：记录上次计数时间，用于防抖
    
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
    
    // 检查按钮长按状态
    // 返回true表示按钮被长按
    // 注意：此方法会自动清除标志
    bool isMasterButtonLongPressed();
    
    // 获取编码器旋转增量
    // 返回自上次调用以来的增量，并重置累计值
    int getEncoderDelta();

    // 中断处理函数需要访问私有成员
    friend void IRAM_ATTR masterButtonISR();
    friend void IRAM_ATTR hmiEncoderISR();
    
private:
    // 静态实例指针
    static SimpleHMI* instance;
};

#endif // SIMPLE_HMI_H