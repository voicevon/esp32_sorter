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
        if (currentState != SimpleHMI::getInstance()->masterButtonPressed) {
            // 保存旧状态用于判断
            bool wasPressed = SimpleHMI::getInstance()->masterButtonPressed;
            
            // 更新状态
            SimpleHMI::getInstance()->masterButtonPressed = currentState;
            SimpleHMI::getInstance()->lastMasterDebounceTime = currentTime;
            
            // 只有当按钮从按下状态变为释放状态时，才认为是完整的按下事件
            if (!currentState && wasPressed) {
                SimpleHMI::getInstance()->masterButtonFlag = true;
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
        if (currentState != SimpleHMI::getInstance()->slaveButtonPressed) {
            // 保存旧状态用于判断
            bool wasPressed = SimpleHMI::getInstance()->slaveButtonPressed;
            
            // 更新状态
            SimpleHMI::getInstance()->slaveButtonPressed = currentState;
            SimpleHMI::getInstance()->lastSlaveDebounceTime = currentTime;
            
            // 只有当按钮从按下状态变为释放状态时，才认为是完整的按下事件
            if (!currentState && wasPressed) {
                SimpleHMI::getInstance()->slaveButtonFlag = true;
            }
        }
    }
}

// 私有构造函数
SimpleHMI::SimpleHMI() : 
    masterButtonPin(MODE_BUTTON_PIN),
    slaveButtonPin(DIAGNOSTIC_BUTTON_PIN),
    masterLEDPin(STATUS_LED1_PIN),
    slaveLEDPin(STATUS_LED2_PIN),
    masterButtonFlag(false),
    slaveButtonFlag(false),
    masterButtonPressed(false),
    slaveButtonPressed(false),
    lastMasterDebounceTime(0),
    lastSlaveDebounceTime(0)
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
    
    // 配置LED引脚为输出模式
    pinMode(masterLEDPin, OUTPUT);
    pinMode(slaveLEDPin, OUTPUT);
    
    // 设置初始LED状态为关闭
    digitalWrite(masterLEDPin, LOW);
    digitalWrite(slaveLEDPin, LOW);
    
    // 初始化按钮状态
    masterButtonPressed = digitalRead(masterButtonPin) == LOW;
    slaveButtonPressed = digitalRead(slaveButtonPin) == LOW;
    
    // 注册中断处理函数（改变状态触发，而不仅仅是下降沿）
    attachInterrupt(digitalPinToInterrupt(masterButtonPin), masterButtonISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(slaveButtonPin), slaveButtonISR, CHANGE);
}

// 检查按钮状态（通过中断标志）- 自动清除标志
bool SimpleHMI::isMasterButtonPressed() {
    bool result = masterButtonFlag;
    // 自动清除标志，确保每个按钮事件只被处理一次
    if (result) {
        masterButtonFlag = false;
    }
    return result;
}

bool SimpleHMI::isSlaveButtonPressed() {
    bool result = slaveButtonFlag;
    // 自动清除标志，确保每个按钮事件只被处理一次
    if (result) {
        slaveButtonFlag = false;
    }
    return result;
}



