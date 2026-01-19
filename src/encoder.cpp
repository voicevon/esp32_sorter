#include "encoder.h"
#include "pins.h"

// 静态成员初始化
Encoder* Encoder::instance = nullptr;

// 编码器逻辑位置范围常量
const int ENCODER_LOGICAL_POSITION_RANGE = 200;

/**
 * 私有构造函数 - 单例模式
 */
Encoder::Encoder() {
    encoderCount = 0;
    previousCount = 0;
    positionChanged = false;
    
    // 初始化回调函数指针为nullptr
    encoderPhaseCallback = nullptr;
    encoderPhaseCallbackContext = nullptr;
    
    // 设置实例指针
    instance = this;
}

/**
 * 获取单例实例的静态方法
 */
Encoder* Encoder::getInstance() {
    if (instance == nullptr) {
        instance = new Encoder();
    }
    return instance;
}

/**
 * 初始化编码器引脚和中断
 */
void Encoder::initialize() {
    // 配置编码器A相、B相、Z相引脚为输入模式
    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    pinMode(ENCODER_PIN_Z, INPUT_PULLUP);
    
    // 配置外部中断处理函数（双中断模式）
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), handleAPhaseInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), handleBPhaseInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_Z), handleZPhaseInterrupt, FALLING);
    
    // 初始化内部状态变量
    encoderCount = 0;
    previousCount = 0;
}

/**
 * 获取当前逻辑位置（0-199）
 */
int Encoder::getCurrentPosition() {
    // 通过count % 200计算得到逻辑位置
    int position = encoderCount % ENCODER_LOGICAL_POSITION_RANGE;
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
    if (!instance) return;
    
    // 保存旧计数值
    long previousCount = instance->encoderCount;
    
    // 双中断模式：读取A相和B相当前状态
    int aPhaseState = digitalRead(ENCODER_PIN_A);
    int bPhaseState = digitalRead(ENCODER_PIN_B);
    
    // 四状态解码算法
    if (aPhaseState == HIGH && bPhaseState == LOW) {
        // A上升沿且B为低，正向旋转
        instance->encoderCount++;
    } else if (aPhaseState == LOW && bPhaseState == HIGH) {
        // A下降沿且B为高，正向旋转
        instance->encoderCount++;
    } else if (aPhaseState == HIGH && bPhaseState == HIGH) {
        // A上升沿且B为高，反向旋转
        instance->encoderCount--;
    } else if (aPhaseState == LOW && bPhaseState == LOW) {
        // A下降沿且B为低，反向旋转
        instance->encoderCount--;
    }
    
    // 检查计数值是否变化
    if (instance->encoderCount != previousCount) {
        // 更新lastCount
        instance->previousCount = instance->encoderCount;
        
        // 设置位置变化标志
        instance->positionChanged = true;
        
        // 调用触发相位回调的方法
        instance->triggerPhaseCallback();
    }
}

/**
 * B相中断处理函数
 */
void Encoder::handleBPhaseInterrupt() {
    if (!instance) return;
    
    // 保存旧计数值
    long previousCount = instance->encoderCount;
    
    // 双中断模式：读取A相和B相当前状态
    int aPhaseState = digitalRead(ENCODER_PIN_A);
    int bPhaseState = digitalRead(ENCODER_PIN_B);
    
    // 四状态解码算法
    if (aPhaseState == HIGH && bPhaseState == HIGH) {
        // B上升沿且A为高，正向旋转
        instance->encoderCount++;
    } else if (aPhaseState == LOW && bPhaseState == LOW) {
        // B下降沿且A为低，正向旋转
        instance->encoderCount++;
    } else if (aPhaseState == LOW && bPhaseState == HIGH) {
        // B上升沿且A为低，反向旋转
        instance->encoderCount--;
    } else if (aPhaseState == HIGH && bPhaseState == LOW) {
        // B下降沿且A为高，反向旋转
        instance->encoderCount--;
    }
    
    // 检查计数值是否变化
    if (instance->encoderCount != previousCount) {
        // 更新lastCount
        instance->previousCount = instance->encoderCount;
        
        // 设置位置变化标志
        instance->positionChanged = true;
        
        // 调用触发相位回调的方法
        instance->triggerPhaseCallback();
    }
}

/**
 * Z相中断处理函数
 */
void Encoder::handleZPhaseInterrupt() {
    if (!instance) return;
    // Z相中断时直接重置计数值
    instance->encoderCount = 0;
    instance->previousCount = 0;
    
    // 设置位置变化标志
    instance->positionChanged = true;
    
    // 调用触发相位回调的方法
    instance->triggerPhaseCallback();
}

/**
 * 私有方法：触发相位回调
 * 避免代码重复，在A相、B相和Z相中断处理中被调用
 */
void Encoder::triggerPhaseCallback() {
    if (encoderPhaseCallback != nullptr) {
        int currentPhase = encoderCount % ENCODER_LOGICAL_POSITION_RANGE;
        encoderPhaseCallback(encoderPhaseCallbackContext, currentPhase);
    }
}

/**
 * 调试信息打印方法
 * 打印编码器的计数值、当前位置等调试信息
 */
void Encoder::printDiagnosticInfo() {
    // 使用函数级静态变量记录上一次的计数值
    static int previousCount = 0;
    
    // 只有当计数值变化时才输出
    if (encoderCount != previousCount) {
        Serial.print("Encoder: Count=");
        Serial.print(encoderCount);
        Serial.print(", Pos=");
        Serial.print(getCurrentPosition());
        Serial.println();
        
        // 更新previousCount为当前count值，避免重复输出
        previousCount = encoderCount;
    }
}