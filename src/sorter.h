#ifndef SORTER_H
#define SORTER_H

#include "encoder.h"
#include "outlet.h"
#include "diameter_scanner.h"
#include "tray_system.h"
#include <ESP32Servo.h>

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

    
private:
    static const uint8_t NUM_OUTLETS = 5;   // 固定安装的出口数量
    
    Encoder* encoder;      // 编码器实例指针（使用单例模式）
    Outlet outlets[NUM_OUTLETS];     // 5个出口数组
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
    
    // 静态回调函数，用于连接编码器相位变化
    static void staticPhaseCallback(void* context, int phase);
    
    // 初始化出口位置
    void initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]);
    
    // 检查并执行出口分配
    void presetOutlets();
};

#endif // SORTER_H