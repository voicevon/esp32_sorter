// 人机交互模块类实现文件
#include "simple_hmi.h"

// 初始化静态成员
SimpleHMI* SimpleHMI::instance = nullptr;

SimpleHMI::SimpleHMI() : 
    masterButtonPin(MODE_BUTTON_PIN),
    slaveButtonPin(DIAGNOSTIC_BUTTON_PIN),
    masterLEDPin(STATUS_LED1_PIN),
    slaveLEDPin(STATUS_LED2_PIN),
    masterButtonPressed(false),
    slaveButtonPressed(false),
    masterLEDState(false),
    slaveLEDState(false),
    lastMasterButtonTime(0),
    lastSlaveButtonTime(0)
{
    instance = this;
}

void SimpleHMI::initialize() {
    // 配置按钮引脚为输入模式（带中断）
    pinMode(masterButtonPin, INPUT_PULLUP);
    pinMode(slaveButtonPin, INPUT_PULLUP);
    
    // 配置LED引脚为输出模式
    pinMode(masterLEDPin, OUTPUT);
    pinMode(slaveLEDPin, OUTPUT);
    
    // 设置初始LED状态
    digitalWrite(masterLEDPin, LOW);
    digitalWrite(slaveLEDPin, LOW);
    
    // 配置中断（下降沿触发）
    attachInterrupt(digitalPinToInterrupt(masterButtonPin), handleMasterButtonInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(slaveButtonPin), handleSlaveButtonInterrupt, FALLING);
    
    Serial.println("Simple HMI initialized");
}

void SimpleHMI::spin_once() {
    unsigned long currentTime = millis();
    
    // 主按钮去抖处理
    if (currentTime - lastMasterButtonTime > DEBOUNCE_TIME) {
        // 检查按钮是否仍然按下（低电平）
        bool currentState = digitalRead(masterButtonPin) == LOW;
        if (currentState) {
            masterButtonPressed = true;
        }
    }
    
    // 从按钮去抖处理
    if (currentTime - lastSlaveButtonTime > DEBOUNCE_TIME) {
        // 检查按钮是否仍然按下（低电平）
        bool currentState = digitalRead(slaveButtonPin) == LOW;
        if (currentState) {
            slaveButtonPressed = true;
        }
    }
}

void SimpleHMI::setLEDState(LED led, bool state) {
    if (led == MASTER_LED) {
        digitalWrite(masterLEDPin, state);
        masterLEDState = state;
    } else if (led == SLAVE_LED) {
        digitalWrite(slaveLEDPin, state);
        slaveLEDState = state;
    }
}



bool SimpleHMI::isButtonPressed(Button button) {
    if (button == MASTER_BUTTON) {
        return masterButtonPressed;
    } else if (button == SLAVE_BUTTON) {
        return slaveButtonPressed;
    }
    return false;
}

void SimpleHMI::clearButtonStates() {
    masterButtonPressed = false;
    slaveButtonPressed = false;
}









// 中断处理函数 - 仅记录时间戳，具体状态由spin_once方法处理
void SimpleHMI::handleMasterButtonInterrupt() {
    if (instance) {
        instance->lastMasterButtonTime = millis();
    }
}

// 中断处理函数 - 仅记录时间戳，具体状态由spin_once方法处理
void SimpleHMI::handleSlaveButtonInterrupt() {
    if (instance) {
        instance->lastSlaveButtonTime = millis();
    }
}