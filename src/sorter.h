#ifndef SORTER_H
#define SORTER_H

#include "encoder.h"
#include "outlet.h"
#include "diameter_scanner.h"
#include "tray_system.h"

class Sorter {
public:
    Sorter();
    
    // 初始化分拣系统
    void initialize();
    

    
    // 采样回调函数（供编码器调用，参数为相位）
    void onPhaseChange(int phase);

    
private:
    static const uint8_t NUM_OUTLETS = 5;   // 固定安装的出口数量
    
    Encoder* encoder;      // 编码器实例指针（使用单例模式）
    Outlet outlets[NUM_OUTLETS];     // 5个出口数组
    DiameterScanner scanner;  // 直径扫描仪实例
    TraySystem traySystem; // 托盘系统实例
    bool running;         // 运行状态标志
    uint8_t divergencePointIndices[NUM_OUTLETS]; // 出口位置数组
    
    // 静态回调函数，用于连接编码器相位变化
    static void staticPhaseCallback(void* context, int phase);
    
    // 初始化出口位置
    void initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]);
    
    // 检查并执行出口分配
    void PresetOutlets();
};

#endif // SORTER_H