#pragma once

#include "encoder.h"
#include "diameter_scanner.h"
#include "tray_manager.h"
#include "outlet.h"
#include "pins.h"
// #include "display_data.h"已移除，不再需要
#include "main.h"
#include "esp32servo.h"
#include "user_interface/simple_hmi.h"

// 前向声明
class SimpleHMI;

// 上升沿和下降沿编码器值的最大数量
#define SORTER_MAX_ENCODER_VALUES 10

// 出口数量定义 - 使用统一的OUTLET_COUNT宏
#define SORTER_NUM_OUTLETS OUTLET_COUNT

// 上料器舵机角度定义
static const int SORTER_RELOADER_OPEN_ANGLE = 90;
static const int SORTER_RELOADER_CLOSE_ANGLE = 180;

// 定义Sorter类
class Sorter {
private:
    // 硬件相关常量
    static const int RELOADER_OPEN_DELAY_MILLIS = 1000;    // 上料器打开延迟（毫秒）
    static const int RELOADER_CLOSE_DELAY_MILLIS = 1000;   // 上料器关闭延迟（毫秒）
    
    // 分流点索引（编码器位置）
    int outletDivergencePoints[SORTER_NUM_OUTLETS];
    
    // 出口对象数组
    Outlet outlets[SORTER_NUM_OUTLETS];
    
    // 直径扫描仪实例指针
    DiameterScanner* scanner;
    
    // 芦笋托盘系统
    TrayManager traySystem;
    
    // 上料器舵机
    Servo reloaderServo;
    
    // 编码器和HMI实例
    Encoder* encoder;
    SimpleHMI* simpleHmi;
    

    
    // 状态标志位（中断安全）
    volatile bool shouldRestartScan;     // 重启扫描标志 - 重置扫描仪状态并启动新的扫描过程
    volatile bool shouldCalculateDiameter; // 计算直径数据标志 - 在特定编码器相位触发直径计算和处理
    volatile bool executeOutlets;         // 执行出口动作标志
    volatile bool resetOutlets;           // 重置出口标志
    volatile bool reloaderOpenRequested;           // 上料器打开标志
    volatile bool reloaderCloseRequested;          // 上料器关闭标志
    
    // 速度计算相关变量
    unsigned long lastSpeedCheckTime;
    long lastEncoderPosition;  // 上次编码器位置
    float lastSpeed;  // 上一次计算的速度值，用于过滤短时间差的异常值
    int lastObjectCount;  // 保留用于向后兼容
    
    // 私有方法
    void prepareOutlets();
    void initializeDivergencePoints(const uint8_t positions[SORTER_NUM_OUTLETS]);
    
public:
    // 构造函数
    Sorter();
    
    // 初始化分拣系统
    void initialize();
    
    // 拆分的任务执行函数
    void processScannerTasks();     // 处理扫描仪相关任务
    void processOutletTasks();      // 处理出口相关任务
    void processReloaderTasks();    // 处理上料器相关任务
    
    // 采样回调函数（供编码器调用，参数为相位）
    void onPhaseChange(int phase);

    // 上料器控制公共方法（用于测试模式）
    void openReloader();
    void closeReloader();
    
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
};
