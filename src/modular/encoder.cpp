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
    zeroCrossCount = 0;
    zeroCrossRawCount = 0;
    forcedZeroCount = 0;
    forcedZeroRawCount = 0;
    phaseOffset = 0;  // 默认无偏移，装机标定后可修改
    
    // 初始化回调函数指针为nullptr
    encoderPhaseCallback = nullptr;
    encoderPhaseCallbackContext = nullptr;

    // 初始化状态缓存
    pinA_state = LOW;
    pinB_state = LOW;
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
    
    // 初始化内部状态变量并同步初始引脚电平
    rawEncoderCount = 0;
    lastEncoderCount = 0;
    zeroCrossCount = 0;
    zeroCrossRawCount = 0;
    forcedZeroCount = 0;
    forcedZeroRawCount = 0;
    
    pinA_state = digitalRead(PIN_ENCODER_A);
    pinB_state = digitalRead(PIN_ENCODER_B);
}

/**
 * 获取当前逻辑位置（0-199）
 */
int Encoder::getCurrentPosition() {
    int position = rawEncoderCount % ENCODER_LOGICAL_POSITION_RANGE;
    if (position < 0) {
        position += ENCODER_LOGICAL_POSITION_RANGE;
    }
    // 叠加零位偏移量，补偿各机器编码器安装位置差异
    return (position + phaseOffset) % ENCODER_LOGICAL_POSITION_RANGE;
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
    Encoder* enc = getInstance();
    int A = digitalRead(PIN_ENCODER_A);
    if (A != enc->pinA_state) {
        int dN = (A == enc->pinB_state) ? 1 : -1;
        if (ENCODER_REVERSE_DIRECTION) {
            enc->rawEncoderCount -= dN;
        } else {
            enc->rawEncoderCount += dN;
        }
        enc->pinA_state = A;
        enc->triggerPhaseCallback();
    }
}

/**
 * B相中断处理函数
 */
void Encoder::handleBPhaseInterrupt() {
    Encoder* enc = getInstance();
    int B = digitalRead(PIN_ENCODER_B);
    if (B != enc->pinB_state) {
        int dN = (enc->pinA_state != B) ? 1 : -1;
        if (ENCODER_REVERSE_DIRECTION) {
            enc->rawEncoderCount -= dN;
        } else {
            enc->rawEncoderCount += dN;
        }
        enc->pinB_state = B;
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
        int raw = rawEncoderCount % ENCODER_LOGICAL_POSITION_RANGE;
        if (raw < 0) raw += ENCODER_LOGICAL_POSITION_RANGE;
        int currentPhase = (raw + phaseOffset) % ENCODER_LOGICAL_POSITION_RANGE;
        encoderPhaseCallback(encoderPhaseCallbackContext, currentPhase);
    }
}