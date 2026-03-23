#pragma once

#include "encoder.h"
#include "diameter_scanner.h"
#include "tray_system.h"
#include "outlet.h"
#include "../config.h"
#include "main.h"
#include "user_interface/simple_hmi.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 定义分拣系统参数
// 注：NUM_OUTLETS 及其它全局物理定义已在 config.h 中由中央管理

// 托盘相关常量已在 TraySystem 中定义，此处仅保留逻辑引用
static const int EMPTY_TRAY = 0;  

// 定义Sorter类
class Sorter {
private:

    
    // 核心硬件资源
    int outletDivergencePoints[NUM_OUTLETS];
    Outlet outlets[NUM_OUTLETS];
    
    // 指针实例
    DiameterScanner* scanner;
    Encoder* encoder;
    SimpleHMI* simpleHmi;
    TraySystem* trayManager;


    
    // 状态触发标志位 (ISR 置位, run() 消费)
    volatile bool flagScanStart;
    volatile bool flagDataLatch;
    volatile bool flagOutletExecute;
    volatile bool flagOutletReset;
    
    // 速度计算相关变量
    unsigned long lastSpeedCheckTime;
    long lastEncoderPosition;  // 上次编码器位置
    float lastSpeed;  // 上一次计算的速度值，用于过滤短时间差的异常值
    int lastObjectCount;  // 保留用于向后兼容
    
    // 私有方法
    void prepareOutlets();
    void restoreOutletConfig(); // Initializes EEPROM and creates outlets
    void initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]);
    
    // 74HC595 同步控制逻辑 (支持 3 级联：LED + Open Coils + Close Coils)
    void updateShiftRegisters();
    uint32_t lastShiftData = 0xFFFFFF; // 记录上次发送的 24 位数据

    // 线程安全互斥锁
    SemaphoreHandle_t mutex;

    
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
    int getLatestDiameter();
    
    // 获取已经输送的托架数量
    int getTransportedTrayCount();
    
    // 获取传送带速度（托架/秒，返回float类型）
    float getConveyorSpeedPerSecond();
    
    // 获取和设置出口直径范围的方法
    int getOutletMinDiameter(uint8_t outletIndex);
    int getOutletMaxDiameter(uint8_t outletIndex);
    void setOutletMinDiameter(uint8_t outletIndex, int minDiameter);
    void setOutletMaxDiameter(uint8_t outletIndex, int maxDiameter);
    
    // 配置持久化
    void saveConfig();
};
