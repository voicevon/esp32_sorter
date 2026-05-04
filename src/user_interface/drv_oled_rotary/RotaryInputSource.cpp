// 精简版人机交互模块实现
#include "RotaryInputSource.h"

// 静态实例初始化
RotaryInputSource* RotaryInputSource::instance = nullptr;

// 中断处理函数 - 实现按下-释放的完整事件检测
void IRAM_ATTR masterButtonISR() {
    unsigned long currentTime = millis();
    // 获取实例指针
    RotaryInputSource* hmi = RotaryInputSource::instance;
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
// HMI 专用编码器中断 - 极速解码 & 降噪方案
void IRAM_ATTR hmiEncoderISR() {
    RotaryInputSource* hmi = RotaryInputSource::instance;
    if (hmi == nullptr) return;
    
    // 软件电平读取
    int s = (gpio_get_level((gpio_num_t)hmi->encoderPinA) << 1) | gpio_get_level((gpio_num_t)hmi->encoderPinB);
    
    if (s != hmi->encoderState) {
        static const int8_t trans[] = {
            0, -1,  1,  2, 
            1,  0,  2, -1, 
           -1,  2,  0,  1, 
            2,  1, -1,  0  
        };
        
        int full_state = (hmi->encoderState << 2) | (s & 0x03);
        int8_t step = trans[full_state & 0x0F];
        
        if (step == 2) {
            hmi->illegalTransitions++;
        } else if (step != 0) {
            hmi->encoderTotalSteps += step;
        }
        
        hmi->encoderState = s;
    }
}

// 私有构造函数
RotaryInputSource::RotaryInputSource() : 
    masterButtonPin(PIN_HMI_BTN),
    encoderPinA(PIN_HMI_ENC_A),
    encoderPinB(PIN_HMI_ENC_B),
    masterButtonClickFlag(false),
    masterButtonLongPressFlag(false),
    masterButtonDownState(false),
    lastMasterDebounceTime(0),
    masterButtonPressStartTime(0),
    encoderTotalSteps(0),
    encoderState(0),
    lastConsumedTotalSteps(0),
    lastHmiStepTime(0),
    illegalTransitions(0)
{
    // 私有构造函数，不应该在外部直接调用
}

// 获取单例实例（懒汉模式）
RotaryInputSource* RotaryInputSource::getInstance() {
    // 注意：在多线程环境中可能需要添加互斥锁
    if (instance == nullptr) {
        instance = new RotaryInputSource();
    }
    return instance;
}

void RotaryInputSource::initialize() {
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

// 获取编码器总步数
int RotaryInputSource::getEncoderTotalSteps() {
    return encoderTotalSteps;
}

// 获取编码器旋转增量（应用 2:1 分频，独立消耗，不干扰他人）
int RotaryInputSource::getEncoderDelta() {
    int delta = 0;
    
    // 读取当前总数
    int currentTotal = encoderTotalSteps;
    int rawDiff = currentTotal - lastConsumedTotalSteps;
    
    // 应用 4:1 分频（1个物理咔哒声 = 4个脉冲 = 1个逻辑步）
    if (abs(rawDiff) >= 4) {
        delta = rawDiff / 4;
        // 更新消耗记录（步进 4 的倍数）
        lastConsumedTotalSteps += delta * 4;
    }
    
    if (delta != 0) {
        Serial.printf("[HMI_ENC] Logical Delta: %d, RawDiff: %d\n", delta, rawDiff);
    }
    
    return delta;
}

// 获取原始旋转增量（独立消费）
int RotaryInputSource::getRawEncoderDelta() {
    // 这里我们可以给 Handler 一个专门的接口，或者 Handler 自己维护 lastCount。
    // 为了方便，这里我们也维护一个隐含的 lastRawCount 吗？
    // 不，最好让 Handler 自己在 update 中维护。
    // 但为了兼容现有的 handler 调用，我们在这里定义一个静态变量来跟踪。
    static int lastRawCounter = 0;
    int current = encoderTotalSteps;
    int diff = current - lastRawCounter;
    lastRawCounter = current;
    return diff;
}

// 检查按钮状态（通过中断标志）- 自动清除标志
bool RotaryInputSource::isMasterButtonPressed() {
    bool result = masterButtonClickFlag;
    if (result) {
        masterButtonClickFlag = false;
    }
    return result;
}

// 检查按钮长按状态 - 自动清除标志
bool RotaryInputSource::isMasterButtonLongPressed() {
    bool result = masterButtonLongPressFlag;
    if (result) {
        masterButtonLongPressFlag = false;
    }
    return result;
}

// 获取干扰统计
uint32_t RotaryInputSource::getIllegalTransitionCount() {
    return illegalTransitions;
}

// ── InputSource 接口实现 ──────────────────────────────────────────────

bool RotaryInputSource::hasIntent() {
    // 非破坏性检查：查看是否有待处理的位移或按钮标志
    int rawDiff = encoderTotalSteps - lastConsumedTotalSteps;
    return masterButtonClickFlag || masterButtonLongPressFlag || (abs(rawDiff) >= 4);
}

UIIntent RotaryInputSource::pollIntent() {
    // 优先处理长按：在菜单逻辑中通常映射为 BACK 或 进入/退出菜单
    if (isMasterButtonLongPressed()) {
        return UIIntent(UIAction::BACK);
    }
    
    // 处理短按：映射为 ACTIVATE (确认)
    if (isMasterButtonPressed()) {
        return UIIntent(UIAction::ACTIVATE);
    }
    
    // 处理旋转：映射为相对导航
    int delta = getEncoderDelta();
    if (delta != 0) {
        return UIIntent(UIAction::NAVIGATE_RELATIVE, delta);
    }
    
    return UIIntent(UIAction::NONE);
}
