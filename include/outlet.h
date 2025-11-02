#ifndef OUTLET_H
#define OUTLET_H

#include <Arduino.h>
#include <ESP32Servo.h>

// 舵机控制参数
#define SERVO_CLOSED_POSITION 90      // 关闭位置（芦笋继续前进）
#define SERVO_OPEN_POSITION_1 0       // 打开位置1（芦笋流出）
#define SERVO_OPEN_POSITION_2 180     // 打开位置2（芦笋流出，备用）
#define SERVO_ACTION_DELAY 1000       // 舵机动作时间（毫秒）
#define SERVO_RECOVERY_DELAY 1500     // 舵机恢复时间（毫秒）

/**
 * 出口类 - 单个出口的控制
 */
class Outlet {
private:
    Servo servo;        // 舵机对象
    int pin;            // 舵机引脚
    bool isActive;      // 激活状态
    bool isInitialized; // 初始化标志
    
public:
    Outlet() : pin(-1), isActive(false), isInitialized(false) {}
    
    void initialize(int servoPin) {
        if (servoPin >= 0) {
            pin = servoPin;
            servo.attach(pin);
            servo.write(SERVO_CLOSED_POSITION);
            isInitialized = true;
        }
    }
    
    void open() {
        if (isInitialized) {
            servo.write(SERVO_OPEN_POSITION_1);
            isActive = true;
        }
    }
    
    void close() {
        if (isInitialized) {
            servo.write(SERVO_CLOSED_POSITION);
            isActive = false;
        }
    }
    
    bool getIsActive() const {
        return isActive;
    }
    
    bool getIsInitialized() const {
        return isInitialized;
    }
    
    int getPin() const {
        return pin;
    }
    
    void test() {
        if (isInitialized) {
            servo.write(SERVO_OPEN_POSITION_1);
            delay(SERVO_ACTION_DELAY);
            servo.write(SERVO_OPEN_POSITION_2);
            delay(SERVO_ACTION_DELAY);
            servo.write(SERVO_CLOSED_POSITION);
        }
    }
};

#endif // OUTLET_H