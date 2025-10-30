#ifndef DIVERTER_CONTROLLER_H
#define DIVERTER_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>

// 舵机控制参数
#define SERVO_STANDARD_POSITION 90     // 标准位置（不分流）
#define SERVO_ASSIGN_POSITION_1 0      // 分配位置1（分流）
#define SERVO_ASSIGN_POSITION_2 180    // 分配位置2（分流，备用）
#define SERVO_ACTION_DELAY 1000        // 舵机动作时间（毫秒）
#define SERVO_RECOVERY_DELAY 1500      // 舵机恢复时间（毫秒）
#define NUM_DIVERTERS 5               // 分支器数量

/**
 * 舵机控制器类 - 管理所有分支器的舵机控制
 */
class DiverterController {
private:
    Servo diverters[NUM_DIVERTERS];    // 舵机对象数组
    int servoPins[NUM_DIVERTERS];      // 舵机引脚数组
    bool activeDiverters[NUM_DIVERTERS]; // 活跃的分支器标志
    unsigned long actionTimers[NUM_DIVERTERS]; // 动作定时器
    bool initialized;                  // 初始化标志

public:
    /**
     * 构造函数
     */
    DiverterController();

    /**
     * 初始化所有舵机
     * @param pins 舵机引脚数组
     */
    void initialize(const int pins[NUM_DIVERTERS]);

    /**
     * 控制指定分支器执行分配动作
     * @param diverterIndex 分支器索引（1-5）
     */
    void activateDiverter(uint8_t diverterIndex);

    /**
     * 控制所有分支器回到标准位置
     */
    void resetAllDiverters();

    /**
     * 控制单个分支器回到标准位置
     * @param diverterIndex 分支器索引（1-5）
     */
    void resetDiverter(uint8_t diverterIndex);

    /**
     * 更新控制逻辑（应在主循环中调用）
     */
    void update();

    /**
     * 测试所有舵机
     */
    void testAllDiverters();

    /**
     * 设置舵机位置
     * @param diverterIndex 分支器索引（1-5）
     * @param angle 角度值
     */
    void setDiverterPosition(uint8_t diverterIndex, int angle);
};

#endif // DIVERTER_CONTROLLER_H