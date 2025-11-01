#ifndef SORTER_CONTROLLER_H
#define SORTER_CONTROLLER_H

#include "carriage_system.h"
#include "outlet_controller.h"

// 默认出口位置
#define DEFAULT_OUTLET_POINT_1 5
#define DEFAULT_OUTLET_POINT_2 10
#define DEFAULT_OUTLET_POINT_3 15
#define DEFAULT_OUTLET_POINT_4 20
#define DEFAULT_OUTLET_POINT_5 25

// 编码器参数
#define ENCODER_PULSES_PER_STEP 10  // 每移动一个索引位置的脉冲数

/**
 * 分拣控制器类 - 系统主控类，协调各个模块
 */
class SorterController {
private:
    OutletController outletController; // 出口控制器
    uint8_t currentPosition;          // 当前传输线位置
    int encoderCount;                 // 编码器计数
    bool isRunning;                   // 系统运行状态
    bool activeOutlets[NUM_OUTLETS];  // 跟踪打开的出口

public:
    /**
     * 构造函数
     */
    SorterController();

    /**
     * 初始化系统
     * @param servoPins 舵机引脚数组
     */
    void initialize(const int servoPins[NUM_OUTLETS]);

    /**
     * 设置出口位置
     * @param positions 出口位置数组
     */
    void setDivergencePoints(const uint8_t positions[NUM_OUTLETS]);

    /**
     * 从单点扫描仪接收直径数据
     * @param diameter 直径值
     */
    void receiveDiameterData(float diameter);

    /**
     * 处理编码器脉冲
     * @param pulseCount 脉冲数
     */
    void handleEncoderPulses(int pulseCount);

    /**
     * 更新系统状态
     */
    void update();

    /**
     * 启动系统
     */
    void start();

    /**
     * 停止系统
     */
    void stop();

    /**
     * 重置系统
     */
    void reset();

    /**
     * 获取当前位置
     * @return 当前位置索引
     */
    uint8_t getCurrentPosition() const;

    /**
     * 运行系统自检
     */
    void runSelfTest();

    /**
     * 测试特定出口
     * @param outletIndex 出口索引（1-5）
     */
    void testOutlet(uint8_t outletIndex);

    /**
     * 模拟移动传输线一个位置
     */
    void moveOnePosition();
    
    /**
     * 显示所有有效托架数据和队列状态（调试用）
     */
    void displayCarriageQueue();
};

#endif // SORTER_CONTROLLER_H