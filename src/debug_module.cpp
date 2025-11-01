// 调试模块类实现文件
#include "debug_module.h"
#include "diagnostic_controller.h"

// 初始化静态成员
DebugModule* DebugModule::instance = nullptr;

DebugModule::DebugModule() : 
    button1Pin(MODE_BUTTON_PIN),
    button2Pin(DIAGNOSTIC_BUTTON_PIN),
    led1Pin(STATUS_LED1_PIN),
    led2Pin(STATUS_LED2_PIN),
    button1Pressed(false),
    button2Pressed(false),
    lastButton1Time(0),
    lastButton2Time(0),
    lastBlinkTime(0),
    blinkState(false),
    led1State(false),
    led2State(false),
    currentSystemMode(MODE_NORMAL),
    debounceTime(50),
    blinkInterval(500)
{
    instance = this;
}

void DebugModule::initialize() {
    // 配置按钮引脚为输入模式（带中断）
    pinMode(button1Pin, INPUT_PULLUP);
    pinMode(button2Pin, INPUT_PULLUP);
    
    // 配置LED引脚为输出模式
    pinMode(led1Pin, OUTPUT);
    pinMode(led2Pin, OUTPUT);
    
    // 设置初始LED状态
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
    
    // 配置中断（下降沿触发）
    attachInterrupt(digitalPinToInterrupt(button1Pin), handleButton1Interrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(button2Pin), handleButton2Interrupt, FALLING);
    
    Serial.println("Debug module initialized");
}

void DebugModule::update() {
    unsigned long currentTime = millis();
    
    // 按钮去抖处理
    if (digitalRead(button1Pin) == HIGH && button1Pressed) {
        if (currentTime - lastButton1Time > debounceTime) {
            // 按钮已经释放并稳定
            button1Pressed = false;
        }
    }
    
    // 按钮2去抖处理
    if (digitalRead(button2Pin) == HIGH && button2Pressed) {
        if (currentTime - lastButton2Time > debounceTime) {
            // 按钮已经释放并稳定
            button2Pressed = false;
        }
    }
    
    // 处理诊断传输线模式的LED慢闪
    if (currentSystemMode == MODE_DIAGNOSE_CONVEYOR) {
        if (currentTime - lastBlinkTime >= blinkInterval) {
            lastBlinkTime = currentTime;
            blinkState = !blinkState;
            digitalWrite(led1Pin, blinkState);
            digitalWrite(led2Pin, blinkState);
        }
    }
    
    // 处理测试模式的LED闪烁
    if (currentSystemMode == MODE_TEST) {
        if (currentTime - lastBlinkTime >= blinkInterval) {
            lastBlinkTime = currentTime;
            blinkState = !blinkState;
            digitalWrite(led1Pin, blinkState);
            digitalWrite(led2Pin, !blinkState);
        }
    }
}

void DebugModule::setLEDState(int ledNumber, bool state) {
    if (ledNumber == 1) {
        digitalWrite(led1Pin, state);
        led1State = state;
    } else if (ledNumber == 2) {
        digitalWrite(led2Pin, state);
        led2State = state;
    }
}

void DebugModule::toggleLED(int ledNumber) {
    if (ledNumber == 1) {
        led1State = !led1State;
        digitalWrite(led1Pin, led1State);
    } else if (ledNumber == 2) {
        led2State = !led2State;
        digitalWrite(led2Pin, led2State);
    }
}

bool DebugModule::isButtonPressed(int buttonNumber) {
    if (buttonNumber == 1) {
        return button1Pressed;
    } else if (buttonNumber == 2) {
        return button2Pressed;
    }
    return false;
}

void DebugModule::clearButtonStates() {
    button1Pressed = false;
    button2Pressed = false;
}

void DebugModule::setSystemMode(int mode) {
    currentSystemMode = mode;
    
    // 停止闪烁状态
    unsigned long currentTime = millis();
    lastBlinkTime = currentTime;
    
    // 根据系统模式设置LED状态
    switch (mode) {
        case MODE_NORMAL:
            setLEDState(1, false);
            setLEDState(2, false);
            break;
        case MODE_DIAGNOSE_ENCODER:
            setLEDState(1, true);
            setLEDState(2, false);
            break;
        case MODE_DIAGNOSE_SCANNER:
            setLEDState(1, false);
            setLEDState(2, true);
            break;
        case MODE_DIAGNOSE_DIVERTER:
            setLEDState(1, true);
            setLEDState(2, true);
            break;
        case MODE_DIAGNOSE_CONVEYOR:
            // 设置LED1和LED2都以1秒的间隔闪烁
            setLEDState(1, true);
            setLEDState(2, true);
            blinkInterval = 1000;
            break;
        case MODE_TEST:
            // 初始状态，闪烁在update中处理
            setLEDState(1, true);
            setLEDState(2, false);
            blinkState = true;
            break;
    }
}

void DebugModule::setDebounceTime(int debounceMs) {
    debounceTime = debounceMs;
}

void DebugModule::setBlinkInterval(int intervalMs) {
    blinkInterval = intervalMs;
}

// 中断处理函数
void DebugModule::handleButton1Interrupt() {
    if (instance) {
        instance->lastButton1Time = millis();
        instance->button1Pressed = true;
    }
}

// 中断处理函数
void DebugModule::handleButton2Interrupt() {
    if (instance) {
        instance->lastButton2Time = millis();
        instance->button2Pressed = true;
    }
}