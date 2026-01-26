// 精简版人机交互模块实现
#include "simple_hmi.h"

// 静态实例初始化
SimpleHMI* SimpleHMI::instance = nullptr;

// 中断处理函数 - 实现按下-释放的完整事件检测
void IRAM_ATTR masterButtonISR() {
    unsigned long currentTime = millis();
    // 防抖动处理
    if (currentTime - SimpleHMI::getInstance()->lastMasterDebounceTime > DEBOUNCE_DELAY) {
        bool currentState = digitalRead(SimpleHMI::getInstance()->masterButtonPin) == LOW;
        
        // 检测按钮状态变化
        if (currentState != SimpleHMI::getInstance()->masterButtonDownState) {
            // 保存旧状态用于判断
            bool wasPressed = SimpleHMI::getInstance()->masterButtonDownState;
            
            // 更新状态
            SimpleHMI::getInstance()->masterButtonDownState = currentState;
            SimpleHMI::getInstance()->lastMasterDebounceTime = currentTime;
            
            // 记录按钮按下的开始时间
            if (currentState) {
                SimpleHMI::getInstance()->masterButtonPressStartTime = currentTime;
            } else if (wasPressed) {
                // 只有当按钮从按下状态变为释放状态时，才认为是完整的按下事件
                unsigned long pressDuration = currentTime - SimpleHMI::getInstance()->masterButtonPressStartTime;
                
                // 检查按下时间是否超过长按阈值
                if (pressDuration >= LONG_PRESS_DELAY) {
                    // 标记为长按事件
                    SimpleHMI::getInstance()->masterButtonLongPressFlag = true;
                } else {
                    // 标记为短按事件
                    SimpleHMI::getInstance()->masterButtonClickFlag = true;
                }
            }
        }
    }
}

void IRAM_ATTR slaveButtonISR() {
    unsigned long currentTime = millis();
    // 防抖动处理
    if (currentTime - SimpleHMI::getInstance()->lastSlaveDebounceTime > DEBOUNCE_DELAY) {
        bool currentState = digitalRead(SimpleHMI::getInstance()->slaveButtonPin) == LOW;
        
        // 检测按钮状态变化
        if (currentState != SimpleHMI::getInstance()->slaveButtonDownState) {
            // 保存旧状态用于判断
            bool wasPressed = SimpleHMI::getInstance()->slaveButtonDownState;
            
            // 更新状态
            SimpleHMI::getInstance()->slaveButtonDownState = currentState;
            SimpleHMI::getInstance()->lastSlaveDebounceTime = currentTime;
            
            // 记录按钮按下的开始时间
            if (currentState) {
                SimpleHMI::getInstance()->slaveButtonPressStartTime = currentTime;
            } else if (wasPressed) {
                // 只有当按钮从按下状态变为释放状态时，才认为是完整的按下事件
                unsigned long pressDuration = currentTime - SimpleHMI::getInstance()->slaveButtonPressStartTime;
                
                // 检查按下时间是否超过长按阈值
                if (pressDuration >= LONG_PRESS_DELAY) {
                    // 标记为长按事件
                    SimpleHMI::getInstance()->slaveButtonLongPressFlag = true;
                } else {
                    // 标记为短按事件
                    SimpleHMI::getInstance()->slaveButtonClickFlag = true;
                }
            }
        }
    }
}

// 私有构造函数
SimpleHMI::SimpleHMI() : 
    masterButtonPin(MODE_BUTTON_PIN),
    slaveButtonPin(DIAGNOSTIC_BUTTON_PIN),
    masterButtonClickFlag(false),
    slaveButtonClickFlag(false),
    masterButtonLongPressFlag(false),
    slaveButtonLongPressFlag(false),
    masterButtonDownState(false),
    slaveButtonDownState(false),
    lastMasterDebounceTime(0),
    lastSlaveDebounceTime(0),
    masterButtonPressStartTime(0),
    slaveButtonPressStartTime(0)
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
    // 配置按钮引脚为输入模式
    pinMode(masterButtonPin, INPUT_PULLUP);
    pinMode(slaveButtonPin, INPUT_PULLUP);
    
    // 初始化按钮状态
    masterButtonDownState = digitalRead(masterButtonPin) == LOW;
    slaveButtonDownState = digitalRead(slaveButtonPin) == LOW;
    
    // 注册中断处理函数（改变状态触发，而不仅仅是下降沿）
    attachInterrupt(digitalPinToInterrupt(masterButtonPin), masterButtonISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(slaveButtonPin), slaveButtonISR, CHANGE);
}

// 检查按钮状态（通过中断标志）- 自动清除标志
bool SimpleHMI::isMasterButtonPressed() {
    bool result = masterButtonClickFlag;
    // 自动清除标志，确保每个按钮事件只被处理一次
    if (result) {
        masterButtonClickFlag = false;
    }
    return result;
}

bool SimpleHMI::isSlaveButtonPressed() {
    bool result = slaveButtonClickFlag;
    // 自动清除标志，确保每个按钮事件只被处理一次
    if (result) {
        slaveButtonClickFlag = false;
    }
    return result;
}

// 检查按钮长按状态
// 返回true表示按钮被长按
// 注意：此方法会自动清除标志
bool SimpleHMI::isMasterButtonLongPressed() {
    bool result = masterButtonLongPressFlag;
    // 自动清除标志，确保每个按钮事件只被处理一次
    if (result) {
        masterButtonLongPressFlag = false;
    }
    return result;
}

bool SimpleHMI::isSlaveButtonLongPressed() {
    bool result = slaveButtonLongPressFlag;
    // 自动清除标志，确保每个按钮事件只被处理一次
    if (result) {
        slaveButtonLongPressFlag = false;
    }
    return result;
}



