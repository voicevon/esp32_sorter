#ifndef OUTLET_H
#define OUTLET_H

#include <Arduino.h>
#include <ESP32Servo.h>

// 出口数量定义 - 项目中统一使用的出口数量
#define OUTLET_COUNT 8

// 舵机控制参数
#define SERVO_CLOSED_POSITION 80      // 关闭位置
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
    int closedPosition;      // 关闭位置
    int openPosition;        // 打开位置
    
public:
    Outlet() : pin(-1), initialized(false), readyToOpenState(false), matchDiameterMin(0), matchDiameterMax(0), closedPosition(SERVO_CLOSED_POSITION), openPosition(SERVO_OPEN_POSITION) {}
    
    // 初始化出口
    void initialize(int servoPin, int minD = 0, int maxD = 0, int closedPos = SERVO_CLOSED_POSITION, int openPos = SERVO_OPEN_POSITION) {
        if (servoPin >= 0) {
            pin = servoPin;
            closedPosition = closedPos;
            openPosition = openPos;
            servo.attach(pin);
            servo.write(closedPosition);
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
    
    // 设置匹配的最小直径
    void setMatchDiameterMin(int minD) {
        matchDiameterMin = minD;
    }
    
    // 设置匹配的最大直径
    void setMatchDiameterMax(int maxD) {
        matchDiameterMax = maxD;
    }
    
    // 设置准备打开状态
    void setReadyToOpen(bool state) {
        readyToOpenState = state;
    }
    
    // 执行出口控制（根据准备打开状态）
    void execute() {
        if (initialized) {
            if (readyToOpenState) {
                servo.write(openPosition);
            } else {
                servo.write(closedPosition);
            }
        }
    }
    
    // 获取关闭位置
    int getClosedPosition() const {
        return closedPosition;
    }
    
    // 获取打开位置
    int getOpenPosition() const {
        return openPosition;
    }
    
    // 设置舵机位置（同时设置打开和关闭位置）
    void setServoPositions(int closedPos, int openPos) {
        closedPosition = closedPos;
        openPosition = openPos;
        // 如果已初始化，根据当前状态更新舵机位置
        if (initialized) {
            if (readyToOpenState) {
                servo.write(openPosition);
            } else {
                servo.write(closedPosition);
            }
        }
    }
    
    // 设置关闭位置（配置模式专用）
    void setClosedPosition(int closedPos) {
        closedPosition = closedPos;
        // 如果已初始化，立即更新舵机位置到新的关闭位置
        if (initialized) {
            servo.write(closedPosition);
        }
    }
    
    // 设置打开位置（配置模式专用）
    void setOpenPosition(int openPos) {
        openPosition = openPos;
        // 如果已初始化，立即更新舵机位置到新的打开位置
        if (initialized) {
            servo.write(openPosition);
        }
    }
};

#endif // OUTLET_H