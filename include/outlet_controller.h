#ifndef OUTLET_CONTROLLER_H
#define OUTLET_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>

// 出口数量
#define NUM_OUTLETS 5                 // 固定安装的出口数量

// 舵机控制参数
#define SERVO_CLOSED_POSITION 90      // 关闭位置（芦笋继续前进）
#define SERVO_OPEN_POSITION_1 0       // 打开位置1（芦笋流出）
#define SERVO_OPEN_POSITION_2 180     // 打开位置2（芦笋流出，备用）
#define SERVO_ACTION_DELAY 1000       // 舵机动作时间（毫秒）
#define SERVO_RECOVERY_DELAY 1500     // 舵机恢复时间（毫秒）

/**
 * 出口控制器类 - 管理所有固定安装的出口的舵机控制
 * 每个出口都有一个舵机，控制出口的打开和关闭状态
 * 当出口打开时，芦笋从该出口流出；当出口关闭时，芦笋继续前进
 */
class OutletController {
private:
    Servo outlets[NUM_OUTLETS];        // 舵机对象数组（每个出口对应一个舵机）
    int servoPins[NUM_OUTLETS];        // 舵机引脚数组
    bool activeOutlets[NUM_OUTLETS];   // 激活的出口标志
    unsigned long actionTimers[NUM_OUTLETS]; // 动作定时器
    bool initialized;                  // 初始化标志

public:
    /**
     * 构造函数
     */
    OutletController();

    /**
     * 初始化所有出口的舵机
     * @param pins 舵机引脚数组
     */
    void initialize(const int pins[NUM_OUTLETS]);

    /**
     * 打开指定出口
     * @param outletIndex 出口索引（1-5）
     */
    void openOutlet(uint8_t outletIndex);

    /**
     * 关闭所有出口
     */
    void closeAllOutlets();

    /**
     * 关闭单个出口
     * @param outletIndex 出口索引（1-5）
     */
    void closeOutlet(uint8_t outletIndex);

    /**
     * 更新控制逻辑（应在主循环中调用）
     * 注意：出口的关闭现在由SorterController基于编码器位置触发，不再通过定时器自动关闭
     */
    void update();

    /**
     * 测试所有出口的舵机
     */
    void testAllOutlets();


};

#endif // DIVERTER_CONTROLLER_H