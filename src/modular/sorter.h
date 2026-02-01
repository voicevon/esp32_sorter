#pragma once

#include "encoder.h"
#include "diameter_scanner.h"
#include "tray_system.h"
#include "outlet.h"
#include "../config.h"
#include "main.h"
#include "esp32servo.h"
#include "user_interface/simple_hmi.h"

// 前向声明
class SimpleHMI;

// 上升沿和下降沿编码器值的最大数量
#define SORTER_MAX_ENCODER_VALUES 10

// 出口数量定义 - 使用统一的NUM_OUTLETS
#define SORTER_NUM_OUTLETS NUM_OUTLETS



// 托盘相关常量
static const uint8_t QUEUE_CAPACITY = 19; // 索引0-18
static const int EMPTY_TRAY = 0;  // 无效直径值，用于表示该位置没有芦笋

// 定义Sorter类
class Sorter {
private:

    
    // 分流点索引（编码器位置）
    int outletDivergencePoints[SORTER_NUM_OUTLETS];
    
    // 出口对象数组
    Outlet outlets[SORTER_NUM_OUTLETS];
    
    // 直径扫描仪实例指针
    DiameterScanner* scanner;
    
    // 编码器和HMI实例
    Encoder* encoder;
    SimpleHMI* simpleHmi;
    
    // 托盘系统实例
    TraySystem* trayManager;


    
    // 状态标志位定义 (FSM States)
    enum SorterState {
        STATE_IDLE,                 // 空闲/等待开始 (Phase 0-1)
        STATE_SCANNING,             // 正在扫描 (Phase 1-110)
        STATE_RESETTING_OUTLETS,    // 复位出口 (Phase 110-120) - 原 resetOutlets
        STATE_CALCULATING_DIAMETER, // 计算直径 (Phase 120-175) - 原 shouldCalculateDiameter
        STATE_EXECUTING_OUTLETS     // 执行分拣 (Phase 175-200) - 原 executeOutlets
    };
    
    // 当前状态
    volatile SorterState currentState;
    
    // 速度计算相关变量
    unsigned long lastSpeedCheckTime;
    long lastEncoderPosition;  // 上次编码器位置
    float lastSpeed;  // 上一次计算的速度值，用于过滤短时间差的异常值
    int lastObjectCount;  // 保留用于向后兼容
    
    // 私有方法
    void prepareOutlets();
    void initializeDivergencePoints(const uint8_t positions[SORTER_NUM_OUTLETS]);
    static uint8_t getCapacity();
    
public:
    // 构造函数
    Sorter();
    
    // 初始化分拣系统
    void initialize();
    
    // 主循环处理函数 (FSM Executor)
    void run();
    
    // 采样回调函数（供编码器调用，参数为相位）
    void onPhaseChange(int phase);


    
    // 出口控制公共方法（用于诊断模式）
    void setOutletState(uint8_t outletIndex, bool open);
    
    // 获取特定出口对象的指针（用于诊断模式）
    Outlet* getOutlet(uint8_t index);

    // 获取显示数据方法已移除，改用专用方法
    
    // 静态回调函数，用于编码器相位变化
    static void onEncoderPhaseChange(void* context, int phase);
    
    // 获取最新直径
    int getLatestDiameter() const;
    
    // 获取已经输送的托架数量
    int getTransportedTrayCount() const;
    
    // 获取传送带速度（托架/秒，返回float类型）
    float getConveyorSpeedPerSecond();
    
    // 获取和设置出口直径范围的方法
    int getOutletMinDiameter(uint8_t outletIndex) const;
    int getOutletMaxDiameter(uint8_t outletIndex) const;
    void setOutletMinDiameter(uint8_t outletIndex, int minDiameter);
    void setOutletMaxDiameter(uint8_t outletIndex, int maxDiameter);
    
    // 获取和设置出口开放/关闭位置的方法
    int getOutletClosedPosition(uint8_t outletIndex) const;
    int getOutletOpenPosition(uint8_t outletIndex) const;
    void setOutletClosedPosition(uint8_t outletIndex, int closedPosition);
    void setOutletOpenPosition(uint8_t outletIndex, int openPosition);
};
