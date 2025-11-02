#ifndef OUTLET_H
#define OUTLET_H

#include <Arduino.h>
#include <ESP32Servo.h>

// 舵机控制参数
#define SERVO_CLOSED_POSITION 90      // 关闭位置
#define SERVO_OPEN_POSITION 0         // 打开位置

/**
 * 出口类 - 单个出口的控制
 */
class Outlet {
private:
    Servo servo;        // 舵机对象
    int pin;            // 舵机引脚
    bool initialized;   // 初始化标志
    bool preOpenState;  // 预开状态标志
    int minDiameter;    // 最小直径
    int maxDiameter;    // 最大直径
    
public:
    Outlet() : pin(-1), initialized(false), preOpenState(false), minDiameter(0), maxDiameter(0) {}
    
    // 初始化出口
    void initialize(int servoPin, int minD = 0, int maxD = 0) {
        if (servoPin >= 0) {
            pin = servoPin;
            servo.attach(pin);
            servo.write(SERVO_CLOSED_POSITION);
            initialized = true;
            minDiameter = minD;
            maxDiameter = maxD;
        }
    }
    
    // 获取最小直径
    int getMinDiameter() const {
        return minDiameter;
    }
    
    // 获取最大直径
    int getMaxDiameter() const {
        return maxDiameter;
    }
    
    // 设置预开状态
    void PreOpen(bool state) {
        preOpenState = state;
    }
    
    // 执行出口控制（根据预开状态）
    void execute() {
        if (initialized) {
            if (preOpenState) {
                servo.write(SERVO_OPEN_POSITION);
            } else {
                servo.write(SERVO_CLOSED_POSITION);
            }
        }
    }
};

#endif // OUTLET_H