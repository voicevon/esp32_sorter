#include "encoder.h"
#include "pins.h"

// 静态成员初始化
Encoder* Encoder::instance = nullptr;

/**
 * 构造函数
 */
Encoder::Encoder() {
    count = 0;
    lastInterruptTime = 0;
    debugEnabled = false;
    reverseRotationDetected = false;
    zeroPositionDetected = false;
    tickCallback = nullptr;
    callbackCount = 0;
    
    // 初始化回调数组
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        callbacks[i] = nullptr;
    }
    
    // 设置实例指针
    instance = this;
}

/**
 * 初始化编码器引脚和中断设置
 */
void Encoder::initialize() {
    // 配置编码器A相、B相、Z相引脚为输入模式
    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    pinMode(ENCODER_PIN_Z, INPUT_PULLUP);
    
    // 配置外部中断处理函数
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), handleAPhaseInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_Z), handleZPhaseInterrupt, FALLING);
    
    // 初始化内部状态变量
    count = 0;
    lastInterruptTime = 0;
    reverseRotationDetected = false;
    zeroPositionDetected = false;
    
    if (debugEnabled) {
        Serial.print("[ENCODER] Initialized with pins: A=");
        Serial.print(ENCODER_PIN_A);
        Serial.print(", B=");
        Serial.print(ENCODER_PIN_B);
        Serial.print(", Z=");
        Serial.println(ENCODER_PIN_Z);
    }
}

/**
 * 注册每相位变化时的回调函数
 */
void Encoder::registerTickCallback(void (*callback)()) {
    tickCallback = callback;
    if (debugEnabled) {
        Serial.println("[ENCODER] Tick callback registered");
    }
}

/**
 * 注销相位变化回调函数
 */
void Encoder::unregisterTickCallback() {
    tickCallback = nullptr;
    if (debugEnabled) {
        Serial.println("[ENCODER] Tick callback unregistered");
    }
}

/**
 * 注册特定位置的回调函数
 */
void Encoder::registerPositionCallback(int position, void (*callback)()) {
    // 检查位置是否在有效范围内
    if (position < 0 || position >= 200) {
        if (debugEnabled) {
            Serial.print("[ENCODER] Invalid position for callback: ");
            Serial.println(position);
        }
        return;
    }
    
    // 检查是否还有空间注册更多回调
    if (callbackCount >= MAX_CALLBACKS) {
        if (debugEnabled) {
            Serial.println("[ENCODER] Maximum number of callbacks reached");
        }
        return;
    }
    
    // 检查是否已经在该位置注册了回调
    for (int i = 0; i < callbackCount; i++) {
        if (callbacks[i]->position == position) {
            // 更新现有回调
            delete callbacks[i];
            callbacks[i] = new PositionCallback{position, callback};
            if (debugEnabled) {
                Serial.print("[ENCODER] Updated position callback at: ");
                Serial.println(position);
            }
            return;
        }
    }
    
    // 添加新的回调
    callbacks[callbackCount] = new PositionCallback{position, callback};
    callbackCount++;
    
    if (debugEnabled) {
        Serial.print("[ENCODER] Registered position callback at: ");
        Serial.println(position);
    }
}

/**
 * 注销特定位置的回调函数
 */
void Encoder::unregisterPositionCallback(int position) {
    for (int i = 0; i < callbackCount; i++) {
        if (callbacks[i]->position == position) {
            // 删除回调
            delete callbacks[i];
            
            // 移动数组中的其他回调
            for (int j = i; j < callbackCount - 1; j++) {
                callbacks[j] = callbacks[j + 1];
            }
            
            callbacks[callbackCount - 1] = nullptr;
            callbackCount--;
            
            if (debugEnabled) {
                Serial.print("[ENCODER] Unregistered position callback at: ");
                Serial.println(position);
            }
            
            return;
        }
    }
    
    if (debugEnabled) {
        Serial.print("[ENCODER] No callback found at position: ");
        Serial.println(position);
    }
}

/**
 * 注销所有回调函数
 */
void Encoder::unregisterAllCallbacks() {
    for (int i = 0; i < callbackCount; i++) {
        delete callbacks[i];
        callbacks[i] = nullptr;
    }
    callbackCount = 0;
    tickCallback = nullptr;
    
    if (debugEnabled) {
        Serial.println("[ENCODER] All callbacks unregistered");
    }
}

/**
 * 检查是否发生反向旋转
 */
bool Encoder::isReverseRotation() {
    return reverseRotationDetected;
}

/**
 * 获取最后中断时间戳
 */
unsigned long Encoder::getLastInterruptTime() {
    return lastInterruptTime;
}

/**
 * 输出编码器当前状态信息
 */
void Encoder::printStatus() {
    Serial.println("[ENCODER] Status:");
    Serial.print("  Count: ");
    Serial.println(count);
    Serial.print("  Current Position: ");
    Serial.println(getCurrentPosition());
    Serial.print("  Reverse Rotation: ");
    Serial.println(reverseRotationDetected ? "YES" : "NO");
    Serial.print("  Zero Position: ");
    Serial.println(zeroPositionDetected ? "DETECTED" : "NOT DETECTED");
    Serial.print("  Callback Count: ");
    Serial.println(callbackCount);
}

/**
 * 启用或禁用调试模式
 */
void Encoder::enableDebug(bool enable) {
    debugEnabled = enable;
    if (debugEnabled) {
        Serial.println("[ENCODER] Debug mode enabled");
    }
}

/**
 * 获取当前逻辑位置（0-199）
 */
int Encoder::getCurrentPosition() {
    // 通过count % 200计算得到逻辑位置
    return count % 200;
}

/**
 * 内部方法：获取当前编码器计数值
 */
long Encoder::getCount() {
    return count;
}

/**
 * 内部方法：获取当前逻辑位置
 */
int Encoder::getPhase() {
    return getCurrentPosition();
}

/**
 * 内部方法：检查是否处于零位位置
 */
bool Encoder::isAtZeroPosition() {
    return zeroPositionDetected;
}

/**
 * 内部方法：重置编码器计数值为零
 */
void Encoder::reset() {
    count = 0;
    zeroPositionDetected = false;
    reverseRotationDetected = false;
    
    if (debugEnabled) {
        Serial.println("[ENCODER] Reset");
    }
}

/**
 * A相中断处理函数
 */
void Encoder::handleAPhaseInterrupt() {
    if (!instance) return;
    
    // 获取当前时间戳
    unsigned long currentTime = micros();
    
    // 读取B相状态
    int bPhaseState = digitalRead(ENCODER_PIN_B);
    
    // 更新最后中断时间
    instance->lastInterruptTime = currentTime;
    
    // 判断旋转方向并更新计数值
    // 假设正常旋转方向为顺时针
    if (bPhaseState == HIGH) {
        // 正向旋转
        instance->count++;
        instance->reverseRotationDetected = false;
    } else {
        // 反向旋转
        instance->count--;
        instance->reverseRotationDetected = true;
        
        if (instance->debugEnabled) {
            Serial.println("[ENCODER] Reverse rotation detected!");
        }
    }
    
    // 执行计数值变化回调
    if (instance->tickCallback) {
        instance->tickCallback();
    }
    
    // 检查是否需要执行位置回调
    int currentPosition = instance->getCurrentPosition();
    for (int i = 0; i < instance->callbackCount; i++) {
        if (instance->callbacks[i]->position == currentPosition) {
            instance->callbacks[i]->callback();
        }
    }
    
    // 调试信息
    if (instance->debugEnabled && (instance->count % 10 == 0)) {
        Serial.print("[ENCODER] Count: ");
        Serial.print(instance->count);
        Serial.print(", Position: ");
        Serial.println(currentPosition);
    }
}

/**
 * Z相中断处理函数
 */
void Encoder::handleZPhaseInterrupt() {
    if (!instance) return;
    
    // 设置零位检测标志
    instance->zeroPositionDetected = true;
    
    if (instance->debugEnabled) {
        Serial.println("[ENCODER] Zero position detected");
    }
}