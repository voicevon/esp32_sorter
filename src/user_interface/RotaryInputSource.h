// 精简版人机交互模块
#ifndef ROTARY_INPUT_SOURCE_H
#define ROTARY_INPUT_SOURCE_H

#include <Arduino.h>
#include "../config.h"

// 防抖动参数
#define DEBOUNCE_DELAY 50  // 防抖动延迟时间（毫秒）
#define LONG_PRESS_DELAY 1000  // 长按检测延迟时间（毫秒）
#define ENCODER_LOCKOUT 20 // 编码器计数锁定时间（毫秒），防止抖动导致双倍计数

// 基本按钮和LED功能定义 - 单例模式实现
class RotaryInputSource {
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
    volatile int encoderTotalSteps;      // 累计旋转总步数（不自动清零）
    volatile int encoderState;           // 记录编码器当前状态逻辑值 (0-3) 
    volatile uint32_t lastHmiStepTime;   // 软件防抖计时
    volatile uint32_t illegalTransitions; // 记录非法的对角线跳转次数（指示有干扰）

    // 上次消耗后的状态（用于 getEncoderDelta 的独立消费逻辑）
    int lastConsumedTotalSteps;
    
    // 私有构造函数，防止外部创建实例
    RotaryInputSource();
    
    // 防止拷贝
    RotaryInputSource(const RotaryInputSource&) = delete;
    RotaryInputSource& operator=(const RotaryInputSource&) = delete;
    
public:
    // 获取单例实例（静态方法）
    static RotaryInputSource* getInstance();
    
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
    
    // 获取编码器总步数（自系统启动起）
    int getEncoderTotalSteps();
    
    // 获取编码器旋转增量（基于 2:1 分频，且本地维护消费记录，不影响他人）
    int getEncoderDelta();

    // 获取原始旋转增量（不分频，不影响他人）
    int getRawEncoderDelta();

    // 获取干扰统计
    uint32_t getIllegalTransitionCount();

    // 中断处理函数需要访问私有成员
    friend void IRAM_ATTR masterButtonISR();
    friend void IRAM_ATTR hmiEncoderISR();
    
private:
    // 静态实例指针
    static RotaryInputSource* instance;
};

#endif // ROTARY_INPUT_SOURCE_H