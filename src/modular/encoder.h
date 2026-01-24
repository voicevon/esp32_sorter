#ifndef ENCODER_H
#define ENCODER_H

#include "Arduino.h"
#include "pins.h"

// 回调函数类型定义 - 只保留相位回调
// 使用void*参数来支持类成员函数回调
typedef void (*PhaseCallback)(void* context, int phase);

/**
 * 编码器类 - 提供位置跟踪、中断处理和回调机制
 */
class Encoder {
private:
    // 核心状态变量
    long rawEncoderCount;            // 编码器计数值
    long lastEncoderCount;           // 上次计数值，用于检测变化
    volatile bool positionChanged; // 位置变化标志（用于OLED显示）
    long zeroCrossCount;            // Z相触发次数（清零次数）
    long zeroCrossRawCount;         // Z相触发时的原始计数值
    int forcedZeroCount;            // 强制清零次数
    long forcedZeroRawCount;        // 强制清零时的原始计数值
    
    // 回调函数指针和上下文
    PhaseCallback encoderPhaseCallback;  // 相位回调
    void* encoderPhaseCallbackContext;    // 回调上下文指针
    
    // 私有构造函数（单例模式）
    Encoder();
    
    // 触发相位回调的私有方法
    void triggerPhaseCallback();
    
public:
    // 静态成员用于中断处理和单例访问
    static Encoder* instance;       // 实例指针
    
    // 获取单例实例的静态方法
    static Encoder* getInstance();
    
    // 初始化编码器引脚和中断
    void initialize();
    
    // 设置回调函数和上下文
    void setPhaseCallback(void* context, PhaseCallback callback);
    
    // 获取当前逻辑位置
    int getCurrentPosition();
    
    // 检查位置是否变化
    bool hasPositionChanged() const { return positionChanged; }
    
    // 重置位置变化标志
    void resetPositionChanged();
    
    // 获取原始计数值
    long getRawCount() const { return rawEncoderCount; }
    
    // 获取清零次数（Z相触发次数）
    long getZeroCrossCount() const { return zeroCrossCount; }
    
    // 获取Z相触发时的原始计数值
    long getZeroCrossRawCount() const { return zeroCrossRawCount; }
    
    // 获取强制清零次数
    int getForcedZeroCount() const { return forcedZeroCount; }
    
    // 获取强制清零时的原始计数值
    long getForcedZeroRawCount() const { return forcedZeroRawCount; }
    
    // 中断处理函数
    static void handleAPhaseInterrupt();  // A相中断
    static void handleBPhaseInterrupt();  // B相中断
    static void handleZPhaseInterrupt();  // Z相中断
};

#endif // ENCODER_H