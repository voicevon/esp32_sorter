#include "encoder.h"
#include "../config.h"

// 静态成员初始化
// Encoder* Encoder::instance = nullptr; // Managed by Singleton template

// 编码器逻辑位置范围常量
const int ENCODER_LOGICAL_POSITION_RANGE = ENCODER_MAX_PHASE;

/**
 * 私有构造函数 - 单例模式
 */
Encoder::Encoder() {
    rawEncoderCount = 0;
    lastEncoderCount = 0;
    positionChanged = false;
    zeroCrossCount = 0;
    zeroCrossRawCount = 0;
    forcedZeroCount = 0;
    forcedZeroRawCount = 0;
    
    // 初始化回调函数指针为nullptr
    encoderPhaseCallback = nullptr;
    encoderPhaseCallbackContext = nullptr;
}

/**
 * 获取单例实例的静态方法 - Managed by Singleton template
 */
// Encoder* Encoder::getInstance() { ... }

/**
 * 初始化编码器引脚和中断
 */
void Encoder::initialize() {
    // 配置编码器A相、B相、Z相引脚为输入模式
    pinMode(PIN_ENCODER_A, INPUT_PULLUP);
    pinMode(PIN_ENCODER_B, INPUT_PULLUP);
    pinMode(PIN_ENCODER_Z, INPUT_PULLUP);
    
    // 配置外部中断处理函数（双中断模式）
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), handleAPhaseInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), handleBPhaseInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_Z), handleZPhaseInterrupt, FALLING);
    
    // 初始化内部状态变量
    rawEncoderCount = 0;
    lastEncoderCount = 0;
    zeroCrossCount = 0;
    zeroCrossRawCount = 0;
    forcedZeroCount = 0;
    forcedZeroRawCount = 0;
}

/**
 * 获取当前逻辑位置（0-199）
 */
int Encoder::getCurrentPosition() {
    // 通过count % 200计算得到逻辑位置
    int position = rawEncoderCount % ENCODER_LOGICAL_POSITION_RANGE;
    // 确保位置为非负数
    if (position < 0) {
        position += ENCODER_LOGICAL_POSITION_RANGE;
    }
    return position;
}

/**
 * 设置相位回调
 */
void Encoder::setPhaseCallback(void* context, PhaseCallback callback) {
    encoderPhaseCallback = callback;
    encoderPhaseCallbackContext = context;
}

/**
 * A相中断处理函数
 */
void Encoder::handleAPhaseInterrupt() {
    // if (!instance) return; // Guaranteed to exist via Singleton
    Encoder* enc = getInstance();
    
    // 保存旧计数值
    long lastCount = enc->rawEncoderCount;
    
    // 双中断模式：读取A相和B相当前状态
    int aPhaseState = digitalRead(PIN_ENCODER_A);
    int bPhaseState = digitalRead(PIN_ENCODER_B);
    
    // 四状态解码算法
    if (aPhaseState == HIGH && bPhaseState == LOW) {
        // A上升沿且B为低，正向旋转
        enc->rawEncoderCount++;
    } else if (aPhaseState == LOW && bPhaseState == HIGH) {
        // A下降沿且B为高，正向旋转
        enc->rawEncoderCount++;
    } else if (aPhaseState == HIGH && bPhaseState == HIGH) {
        // A上升沿且B为高，反向旋转
        enc->rawEncoderCount--;
    } else if (aPhaseState == LOW && bPhaseState == LOW) {
        // A下降沿且B为低，反向旋转
        enc->rawEncoderCount--;
    }
    
    // 检查计数值是否变化
    if (enc->rawEncoderCount != lastCount) {
        // 更新lastCount
        enc->lastEncoderCount = enc->rawEncoderCount;
        
        // 设置位置变化标志
        enc->positionChanged = true;
        
        // 调用触发相位回调的方法
        enc->triggerPhaseCallback();
    }
}

/**
 * B相中断处理函数
 */
void Encoder::handleBPhaseInterrupt() {
    // if (!instance) return;
    Encoder* enc = getInstance();
    
    // 保存旧计数值
    long lastCount = enc->rawEncoderCount;
    
    // 双中断模式：读取A相和B相当前状态
    int aPhaseState = digitalRead(PIN_ENCODER_A);
    int bPhaseState = digitalRead(PIN_ENCODER_B);
    
    // 四状态解码算法
    if (aPhaseState == HIGH && bPhaseState == HIGH) {
        // B上升沿且A为高，正向旋转
        enc->rawEncoderCount++;
    } else if (aPhaseState == LOW && bPhaseState == LOW) {
        // B下降沿且A为低，正向旋转
        enc->rawEncoderCount++;
    } else if (aPhaseState == LOW && bPhaseState == HIGH) {
        // B上升沿且A为低，反向旋转
        enc->rawEncoderCount--;
    } else if (aPhaseState == HIGH && bPhaseState == LOW) {
        // B下降沿且A为高，反向旋转
        enc->rawEncoderCount--;
    }
    
    // 检查计数值是否变化
    if (enc->rawEncoderCount != lastCount) {
        // 更新lastCount
        enc->lastEncoderCount = enc->rawEncoderCount;
        
        // 设置位置变化标志
        enc->positionChanged = true;
        
        // 调用触发相位回调的方法
        enc->triggerPhaseCallback();
    }
}

/**
 * Z相中断处理函数
 */
void Encoder::handleZPhaseInterrupt() {
    // if (!instance) return;
    Encoder* enc = getInstance();
    
    // 增加清零次数计数
    enc->zeroCrossCount++;
    
    // 记录Z相触发时的原始计数值
    enc->zeroCrossRawCount = enc->rawEncoderCount;
    
    // 当清零时，如果计数器对400取模余数不是0，则强行设为0
    if (enc->rawEncoderCount % 400 != 0) {
        // 记录强制清零时的原始计数值
        enc->forcedZeroRawCount = enc->rawEncoderCount;
        // 增加强制清零计数
        enc->forcedZeroCount++;
        // 直接设为0
        enc->rawEncoderCount = 0;
    }
    
    // 设置位置变化标志
    enc->positionChanged = true;
    
    // 调用触发相位回调的方法，传递特殊相位值255表示Z相信号
    if (enc->encoderPhaseCallback != nullptr) {
        enc->encoderPhaseCallback(enc->encoderPhaseCallbackContext, 255);
    }
}

/**
 * 私有方法：触发相位回调
 * 避免代码重复，在A相、B相和Z相中断处理中被调用
 */
void Encoder::triggerPhaseCallback() {
    if (encoderPhaseCallback != nullptr) {
        int currentPhase = rawEncoderCount % ENCODER_LOGICAL_POSITION_RANGE;
        encoderPhaseCallback(encoderPhaseCallbackContext, currentPhase);
    }
}

/**
 * 重置位置变化标志
 * 在显示位置信息后调用，避免重复显示
 */
void Encoder::resetPositionChanged() {
    positionChanged = false;
}