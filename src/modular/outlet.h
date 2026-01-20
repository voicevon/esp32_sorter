#ifndef OUTLET_H
#define OUTLET_H

#include <Arduino.h>
#include <ESP32Servo.h>

// 出口数量定义 - 项目中统一使用的出口数量
#define OUTLET_COUNT 8

// 舵机控制参数
#define SERVO_CLOSED_POSITION 135      // 关闭位置
#define SERVO_OPEN_POSITION 0         // 打开位置

/**
 * 出口类 - 单个出口的控制
 */
class Outlet {
private:
    Servo servo;        // 舵机对象
    int pin;            // 舵机引脚
    bool initialized;   // 初始化标志
    bool readyToOpenState;  // 准备打开状态标志
    int matchDiameterMin;    // 匹配该出口的最小直径
    int matchDiameterMax;    // 匹配该出口的最大直径
    
public:
    Outlet() : pin(-1), initialized(false), readyToOpenState(false), matchDiameterMin(0), matchDiameterMax(0) {}
    
    // 初始化出口
    void initialize(int servoPin, int minD = 0, int maxD = 0) {
        if (servoPin >= 0) {
            pin = servoPin;
            servo.attach(pin);
            servo.write(SERVO_CLOSED_POSITION);
            initialized = true;
            matchDiameterMin = minD;
            matchDiameterMax = maxD;
        }
    }
    
    // 获取匹配的最小直径
    int getMatchDiameterMin() const {
        return matchDiameterMin;
    }
    
    // 获取匹配的最大直径
    int getMatchDiameterMax() const {
        return matchDiameterMax;
    }
    
    // 设置准备打开状态
    void setReadyToOpen(bool state) {
        readyToOpenState = state;
    }
    
    // 执行出口控制（根据准备打开状态）
    void execute() {
        if (initialized) {
            if (readyToOpenState) {
                servo.write(SERVO_OPEN_POSITION);
            } else {
                servo.write(SERVO_CLOSED_POSITION);
            }
        }
    }
};

#endif // OUTLET_H