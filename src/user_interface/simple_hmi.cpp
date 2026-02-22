// 精简版人机交互模块实现
#include "simple_hmi.h"

// 静态实例初始化
SimpleHMI* SimpleHMI::instance = nullptr;

// 中断处理函数 - 实现按下-释放的完整事件检测
void IRAM_ATTR masterButtonISR() {
    unsigned long currentTime = millis();
    // 获取实例指针
    SimpleHMI* hmi = SimpleHMI::instance;
    if (hmi == nullptr) return;
    
    // 读取当前按钮状态
    bool currentState = digitalRead(hmi->masterButtonPin) == LOW;
    
    // 防抖动处理
    if (currentTime - hmi->lastMasterDebounceTime > DEBOUNCE_DELAY) {
        // 检测按钮状态变化
        if (currentState != hmi->masterButtonDownState) {
            // 保存旧状态用于判断
            bool wasPressed = hmi->masterButtonDownState;
            
            // 更新状态
            hmi->masterButtonDownState = currentState;
            hmi->lastMasterDebounceTime = currentTime;
            
            // 记录按钮按下的开始时间
            if (currentState) {
                hmi->masterButtonPressStartTime = currentTime;
            } else if (wasPressed) {
                // 只有当按钮从按下状态变为释放状态时，才认为是完整的按下事件
                unsigned long pressDuration = currentTime - hmi->masterButtonPressStartTime;
                
                // 检查按下时间是否超过长按阈值
                if (pressDuration >= LONG_PRESS_DELAY) {
                    // 标记为长按事件
                    hmi->masterButtonLongPressFlag = true;
                } else {
                    // 标记为短按事件
                    hmi->masterButtonClickFlag = true;
                }
            }
        }
    }
}

    // 临时的按钮按下状态
// HMI 专用编码器中断 - 专业级状态机解码方案
// 此方案模仿工业编码器逻辑，要求状态必须在灰度码序列中移动
void IRAM_ATTR hmiEncoderISR() {
    SimpleHMI* hmi = SimpleHMI::instance;
    if (hmi == nullptr) return;
    
    // 读取当前 A/B 状态 (0-3)
    int s = (digitalRead(hmi->encoderPinA) << 1) | digitalRead(hmi->encoderPinB);
    
    if (s != hmi->encoderState) {
        // 全相位转换表 (4x 分辨率)
        // 此表能自动抵消抖动：3 -> 2 -> 3 会产生 -1 + 1 = 0
        static const int8_t trans[] = {
            0, -1,  1,  0,
            1,  0,  0, -1,
           -1,  0,  0,  1,
            0,  1, -1,  0
        };
        
        int full_state = (hmi->encoderState << 2) | s;
        hmi->encoderDelta += trans[full_state & 0x0F];
        
        hmi->encoderState = s;
    }
}

// 私有构造函数
SimpleHMI::SimpleHMI() : 
    masterButtonPin(PIN_HMI_BTN),
    encoderPinA(PIN_HMI_ENC_A),
    encoderPinB(PIN_HMI_ENC_B),
    masterButtonClickFlag(false),
    masterButtonLongPressFlag(false),
    masterButtonDownState(false),
    lastMasterDebounceTime(0),
    masterButtonPressStartTime(0),
    lastEncoderLevelA(HIGH),
    encoderDelta(0),
    encoderState(0),
    lastEncoderInterruptTime(0)
{
    // 私有构造函数，不应该在外部直接调用
}

// 获取单例实例（懒汉模式）
SimpleHMI* SimpleHMI::getInstance() {
    // 注意：在多线程环境中可能需要添加互斥锁
    if (instance == nullptr) {
        instance = new SimpleHMI();
    }
    return instance;
}

void SimpleHMI::initialize() {
    // 配置按钮和编码器引脚为输入上拉模式
    pinMode(masterButtonPin, INPUT_PULLUP);
    pinMode(encoderPinA, INPUT_PULLUP);
    pinMode(encoderPinB, INPUT_PULLUP);
    
    // 初始化按钮状态与编码器状态
    masterButtonDownState = digitalRead(masterButtonPin) == LOW;
    encoderState = (digitalRead(encoderPinA) << 1) | digitalRead(encoderPinB);
    
    // 注册中断处理函数
    attachInterrupt(digitalPinToInterrupt(masterButtonPin), masterButtonISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(encoderPinA), hmiEncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(encoderPinB), hmiEncoderISR, CHANGE);
}

// 获取编码器旋转增量
int SimpleHMI::getEncoderDelta() {
    int delta = 0;
    // 简单的临界区保护，避免读取时被中断修改
    noInterrupts();
    delta = encoderDelta;
    encoderDelta = 0;
    interrupts();

    if (delta != 0) {
        Serial.printf("[HMI_ENC] Delta: %d\n", delta);
    }
    
    return delta;
}

// 检查按钮状态（通过中断标志）- 自动清除标志
bool SimpleHMI::isMasterButtonPressed() {
    bool result = masterButtonClickFlag;
    if (result) {
        masterButtonClickFlag = false;
    }
    return result;
}

// 检查按钮长按状态 - 自动清除标志
bool SimpleHMI::isMasterButtonLongPressed() {
    bool result = masterButtonLongPressFlag;
    if (result) {
        masterButtonLongPressFlag = false;
    }
    return result;
}
