#ifndef ENCODER_H
#define ENCODER_H

#include "Arduino.h"
#include "pins.h"

// 编码器引脚定义（使用pins.h中定义的引脚）
// 直接使用pins.h中定义的常量，避免重复定义

/**
 * 编码器类 - 用于封装ESP32分拣系统中的编码器模块功能
 * 提供精确的位置跟踪、位置回调事件机制和中断处理能力
 */
class Encoder {
private:
    // 常量定义
    static const int MAX_CALLBACKS = 10;  // 最大回调函数数量
    
    // 计数值和状态
    long count;                      // 编码器计数值
    unsigned long lastInterruptTime; // 最后中断时间戳
    bool debugEnabled;               // 调试模式标志
    bool reverseRotationDetected;    // 反向旋转检测标志（报警状态）
    bool zeroPositionDetected;       // 零位检测标志
    
    // 回调函数相关
    void (*tickCallback)();          // 每计数值变化时的回调函数指针
    
    // 位置回调结构体
    struct PositionCallback {
        int position;                // 逻辑位置
        void (*callback)();          // 回调函数
    };
    
    // 位置回调数组
    PositionCallback* callbacks[MAX_CALLBACKS];
    int callbackCount;               // 已注册回调函数数量
    
    // 私有方法
    long getCount();                 // 获取当前编码器计数值（内部使用）
    int getPhase();                  // 获取当前逻辑位置（内部使用）
    bool isAtZeroPosition();         // 检查是否处于零位位置（内部使用）
    void reset();                    // 重置编码器计数值为零（内部使用）
    
public:
    // 静态成员
    static Encoder* instance;       // 指向Encoder类实例的静态指针，用于中断处理函数访问
    
    /**
     * 构造函数
     */
    Encoder();
    
    /**
     * 初始化编码器引脚和中断设置
     */
    void initialize();
    
    /**
     * 注册每相位变化时的回调函数（用于激光扫描仪、直径扫描仪等采样）
     * @param callback 回调函数指针
     */
    void registerTickCallback(void (*callback)());
    
    /**
     * 注销相位变化回调函数
     */
    void unregisterTickCallback();
    
    /**
     * 注册特定位置的回调函数
     * @param position 逻辑位置（0-199）
     * @param callback 回调函数指针
     */
    void registerPositionCallback(int position, void (*callback)());
    
    /**
     * 注销特定位置的回调函数
     * @param position 逻辑位置（0-199）
     */
    void unregisterPositionCallback(int position);
    
    /**
     * 注销所有回调函数
     */
    void unregisterAllCallbacks();
    
    /**
     * 检查是否发生反向旋转（报警状态）
     * @return 如果检测到反向旋转返回true
     */
    bool isReverseRotation();
    
    /**
     * 获取最后中断时间戳
     * @return 最后中断时间戳
     */
    unsigned long getLastInterruptTime();
    
    /**
     * 输出编码器当前状态信息
     */
    void printStatus();
    
    /**
     * 启用或禁用调试模式
     * @param enable 是否启用调试模式
     */
    void enableDebug(bool enable);
    
    /**
     * 获取当前逻辑位置（0-199）
     * @return 当前逻辑位置
     */
    int getCurrentPosition();
    
    // 中断处理函数
    static void handleAPhaseInterrupt();  // A相中断处理函数
    static void handleZPhaseInterrupt();  // Z相中断处理函数
};

#endif // ENCODER_H