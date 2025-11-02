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
    long count;                      // 编码器计数值
    long lastCount;                  // 上次计数值，用于检测变化
    
    // 回调函数指针和上下文
    PhaseCallback phaseCallback;  // 相位回调
    void* phaseCallbackContext;    // 回调上下文指针
    
    // 私有构造函数（单例模式）
    Encoder();
    
    // 移除了triggerPhaseCallback方法，将实现直接放到调用处
    
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
    
    // 中断处理函数
    static void handleAPhaseInterrupt();  // A相中断
    static void handleBPhaseInterrupt();  // B相中断
    static void handleZPhaseInterrupt();  // Z相中断
    
    // 调试信息打印方法
    void printout();
};;

#endif // ENCODER_H