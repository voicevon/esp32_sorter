#ifndef SORTER_H
#define SORTER_H

#include "encoder.h"
#include "outlet.h"
#include "diameter_scanner.h"
#include "tray_system.h"
#include "simple_hmi.h"
#include <ESP32Servo.h>
#include "display_data.h"

// 上料器舵机角度定义
#define RELOADER_OPEN_ANGLE 90     // 上料器开启角度
#define RELOADER_CLOSE_ANGLE 0     // 上料器关闭角度

class Sorter {
public:
    // 构造函数
    Sorter();
    
    // 初始化分拣系统
    void initialize();
    
    // 主循环执行函数
    void spinOnce();
    
    // 采样回调函数（供编码器调用，参数为相位）
    void onPhaseChange(int phase);
    
    // 上料器控制公共方法（用于测试模式）
    void openReloader();
    void closeReloader();
    
    // 出口控制公共方法（用于诊断模式）
    void setOutletState(uint8_t outletIndex, bool open);
    
    // 获取出口数量的方法
    uint8_t getOutletCount() const { return NUM_OUTLETS; }
    
    // 设置直径扫描仪日志级别的方法（用于诊断模式）
    void setScannerLogLevel(LoggerLevel level) {
        scanner.setLogLevel(level);
    }
    
    // 获取直径扫描仪物体计数的方法（用于诊断模式）
    int getScannerObjectCount() const { return scanner.getTotalObjectCount(); }
    
    // 获取直径扫描仪当前直径值的方法（用于诊断模式和正常模式）
    int getScannerDiameter() const { return scanner.getDiameterAndStop(); }
    
    // 获取最新检测到的直径值（用于正常模式显示）
    int getLatestDiameter() const;
    
    // 获取已识别的物体数量（用于正常模式显示）
    int getIdentifiedCount() const { return scanner.getTotalObjectCount(); }
    
    // 获取传送带前进的托架数量（用于正常模式显示）
    int getTrayCount() const;
    
    // 获取分拣速度（根/小时）（用于正常模式显示）
    int getSortingSpeed();
    
    // 获取分拣速度（根/秒）（用于正常模式显示）
    int getSortingSpeedPerSecond();
    
    // 获取分拣速度（根/分钟）（用于正常模式显示）
    int getSortingSpeedPerMinute();
    
    // 获取IO状态（用于诊断模式子模式1）
    String getIOStatus();
    
    // 显示IO状态（用于诊断模式子模式1）
    void displayIOStatus();
    
    // 获取原始直径值（用于诊断模式子模式2）
    String getRawDiameters();
    
    // 显示原始直径值（用于诊断模式子模式2）
    void displayRawDiameters();
    
    // 获取显示数据（用于 UserInterface）
    DisplayData getDisplayData(SystemMode currentMode, int normalSubMode = 0, int encoderSubMode = 0, int outletSubMode = 0);

    
private:
    static const uint8_t NUM_OUTLETS = 8;   // 固定安装的出口数量
    
    Encoder* encoder;      // 编码器实例指针（使用单例模式）
    SimpleHMI* hmi;        // HMI实例指针（使用单例模式）
    Outlet outlets[NUM_OUTLETS];     // 8个出口数组
    DiameterScanner scanner;  // 直径扫描仪实例
    TraySystem traySystem; // 托盘系统实例
    bool running;         // 运行状态标志
    uint8_t divergencePointIndices[NUM_OUTLETS]; // 出口位置数组
    Servo reloaderServo;   // 上料器舵机实例
    
    // 状态标志位（中断安全）
    volatile bool resetScannerFlag;
    volatile bool processScanDataFlag;
    volatile bool executeOutletsFlag;
    volatile bool resetOutletsFlag;
    volatile bool reloaderOpenFlag;    // 上料器开启标志
    volatile bool reloaderCloseFlag;   // 上料器关闭标志
    
    // 用于速度计算的变量
    unsigned long lastSpeedCheckTime;  // 上次速度检查时间
    int lastObjectCount;               // 上次物体计数
    
    // 静态回调函数，用于连接编码器相位变化
    static void staticPhaseCallback(void* context, int phase);
    
    // 初始化出口位置
    void initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]);
    
    // 检查并执行出口分配
    void presetOutlets();
};

#endif // SORTER_H