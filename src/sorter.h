#pragma once

#include "encoder.h"
#include "diameter_scanner.h"
#include "asparagus_tray_manager.h"
#include "outlet.h"
#include "pins.h"
#include "display_data.h"
#include "main.h"
#include "esp32servo.h"
#include "simple_hmi.h"

// 前向声明
class SimpleHMI;

// 上升沿和下降沿编码器值的最大数量
#define SORTER_MAX_ENCODER_VALUES 10

// 出口数量定义
static const int SORTER_NUM_OUTLETS = 10;

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
    int divergencePointIndices[SORTER_NUM_OUTLETS];
    
    // 出口对象数组
    Outlet outlets[SORTER_NUM_OUTLETS];
    
    // 直径扫描仪实例指针
    DiameterScanner* scanner;
    
    // 芦笋托盘系统
    AsparagusTrayManager traySystem;
    
    // 上料器舵机
    Servo reloaderServo;
    
    // 编码器和HMI实例
    Encoder* encoder;
    SimpleHMI* hmi;
    

    
    // 状态标志位（中断安全）
    volatile bool restartScanFlag;          // 重启扫描标志 - 重置扫描仪状态并启动新的扫描过程
    volatile bool calculateDiameterFlag;      // 计算直径数据标志 - 在特定编码器相位触发直径计算和处理
    volatile bool executeOutletsFlag;         // 执行出口动作标志
    volatile bool resetOutletsFlag;           // 重置出口标志
    volatile bool reloaderOpenFlag;           // 上料器打开标志
    volatile bool reloaderCloseFlag;          // 上料器关闭标志
    
    // 速度计算相关变量
    unsigned long lastSpeedCheckTime;
    int lastObjectCount;
    
    // 私有方法
    void presetOutlets();
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

    // 获取显示数据（用于OLED）
    DisplayData getDisplayData(SystemMode currentMode, int normalSubMode, int encoderSubMode, int outletSubMode);
    
    // 静态回调函数，用于编码器相位变化
    static void staticPhaseCallback(void* context, int phase);
    
    // 获取最新直径
    int getLatestDiameter() const;
    
    // 获取已经输送的托架数量
    int getTransportedTrayCount() const;
    
    // 获取每秒分拣速度（返回float类型）
    float getSortingSpeedPerSecond();
};
