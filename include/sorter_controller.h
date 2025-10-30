#ifndef SORTER_CONTROLLER_H
#define SORTER_CONTROLLER_H

#include "carriage_system.h"
#include "diverter_controller.h"

// 默认分支点位置
#define DEFAULT_DIVERGENCE_POINT_1 5
#define DEFAULT_DIVERGENCE_POINT_2 10
#define DEFAULT_DIVERGENCE_POINT_3 15
#define DEFAULT_DIVERGENCE_POINT_4 20
#define DEFAULT_DIVERGENCE_POINT_5 25

// 编码器参数
#define ENCODER_PULSES_PER_STEP 10  // 每移动一个索引位置的脉冲数

/**
 * 分拣控制器类 - 系统主控类，协调各个模块
 */
class SorterController {
private:
    CarriageManager carriageManager;      // 托架管理器
    DiverterController diverterController; // 分支器控制器
    uint8_t currentPosition;              // 当前传输线位置
    int encoderCount;                     // 编码器计数
    bool isRunning;                       // 系统运行状态

public:
    /**
     * 构造函数
     */
    SorterController();

    /**
     * 初始化系统
     * @param servoPins 舵机引脚数组
     */
    void initialize(const int servoPins[NUM_DIVERTERS]);

    /**
     * 设置分支点位置
     * @param positions 分支点位置数组
     */
    void setDivergencePoints(const uint8_t positions[NUM_DIVERTERS]);

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
     * 测试特定分支器
     * @param diverterIndex 分支器索引（1-5）
     */
    void testDiverter(uint8_t diverterIndex);

    /**
     * 模拟移动传输线一个位置
     */
    void moveOnePosition();
};

#endif // SORTER_CONTROLLER_H