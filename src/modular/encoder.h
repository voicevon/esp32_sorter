#ifndef ENCODER_H
#define ENCODER_H

#include "Arduino.h"
#include "../config.h"

#include "../utils/singleton.h"

// 回调函数类型定义 - 只保留相位回调
// 使用void*参数来支持类成员函数回调
typedef void (*PhaseCallback)(void* context, int phase);

/**
 * 编码器类 - 提供位置跟踪、中断处理和回调机制
 */
class Encoder : public Singleton<Encoder> {
    friend class Singleton<Encoder>;
private:
    // 核心状态变量
    long rawEncoderCount;            // 编码器计数值
    long lastEncoderCount;           // 上次计数值，用于检测变化
    long zeroCrossCount;            // Z相触发次数（清零次数）
    long zeroCrossRawCount;         // Z相触发时的原始计数值
    int forcedZeroCount;            // 强制清零次数
    long forcedZeroRawCount;        // 强制清零时的原始计数值
    int phaseOffset;                // 零位偏移量：补偿各机器编码器安装位置差异
    
    // 引脚状态缓存（参考 SimpleFOC 优化）
    volatile int pinA_state;        // A相上一个状态
    volatile int pinB_state;        // B相上一个状态
    
    // 回调函数指针和上下文
    PhaseCallback encoderPhaseCallback;  // 相位回调
    void* encoderPhaseCallbackContext;    // 回调上下文指针
    
    // 私有构造函数（单例模式）
    Encoder();
    
    // 触发相位回调的私有方法
    void triggerPhaseCallback();
    
public:
    // initialize方法移至public
    
    // 初始化编码器引脚和中断
    void initialize();
    
    // 设置回调函数和上下文
    void setPhaseCallback(void* context, PhaseCallback callback);
    
    // 获取当前逻辑位置（已叠加 phaseOffset，对外统一使用此接口）
    int getCurrentPosition();

    // 设置零位偏移量（装机标定时调用一次，范围 0~199）
    void setPhaseOffset(int offset) { phaseOffset = offset % ENCODER_MAX_PHASE; }

    // 获取当前零位偏移量
    int getPhaseOffset() const { return phaseOffset; }

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