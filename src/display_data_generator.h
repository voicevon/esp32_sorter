#pragma once

#include "display_data.h"
#include "main.h"
#include "diameter_scanner.h"
#include "asparagus_tray_manager.h"
#include "encoder.h"

/**
 * 显示数据生成器类
 * 负责为不同模式生成显示数据
 */
class DisplayDataGenerator {
private:
    // 直径扫描仪实例指针
    DiameterScanner* scanner;
    
    // 芦笋托盘系统实例
    AsparagusTrayManager& traySystem;
    
    // 编码器实例
    Encoder* encoder;
    
    // 速度计算相关变量
    unsigned long lastSpeedCheckTime;
    int lastObjectCount;
    
public:
    /**
     * 构造函数
     * @param traySystem 芦笋托盘系统实例引用
     */
    DisplayDataGenerator(AsparagusTrayManager& traySystem);
    
    /**
     * 初始化显示数据生成器
     */
    void initialize();
    
    /**
     * 生成显示数据
     * @param currentMode 当前系统模式
     * @param normalSubMode 正常模式子模式
     * @param encoderSubMode 编码器诊断模式子模式
     * @param outletSubMode 出口诊断模式子模式
     * @return 显示数据
     */
    DisplayData getDisplayData(SystemMode currentMode, int normalSubMode, int encoderSubMode, int outletSubMode);
    
private:
    /**
     * 获取最新直径
     * @return 最新直径
     */
    int getLatestDiameter() const;
    
    /**
     * 获取已经输送的托架数量
     * @return 已经输送的托架数量
     */
    int getTransportedTrayCount() const;
    
    /**
     * 获取每秒分拣速度
     * @return 每秒分拣速度（float类型）
     */
    float getSortingSpeedPerSecond();
};
