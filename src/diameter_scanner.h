#ifndef DIAMETER_SCANNER_H
#define DIAMETER_SCANNER_H

#include <Arduino.h>
#include "pins.h"

// 日志级别枚举
enum LoggerLevel {
    LOG_LEVEL_SILENT = 0,  // 静默模式，不输出任何日志
    LOG_LEVEL_INFO = 1,    // 信息模式，输出基本信息
    LOG_LEVEL_DEBUG = 2    // 调试模式，输出详细调试信息
};

class DiameterScanner {
private:
    // 引脚定义（4个扫描点）
    int scannerPins[4];
    bool isScanning;
    // 高电平采样计数（每个扫描点）
    int highLevelCounts[4];
    
    // 统计次数（计算实际物体数量）
    int objectCount;
    
    // 上一次的传感器状态（每个扫描点）
    bool lastSensorStates[4];
    
    // 是否正在采样（每个扫描点）
    bool isSampling[4];
    
    // 计算得到的直径值（整数）
    int calculatedDiameter;
    
    // 日志级别
    LoggerLevel logLevel;

public:
    // 构造函数
    DiameterScanner();
    
    // 初始化引脚和缓冲区
    void initialize();
    
    // 重置状态和缓冲区
    void start();
    
    // 采样传感器状态（根据相位进行采样）
    void sample(int phase);
    
    // 获取计算的直径值（整数）并停止扫描
    int getDiameterAndStop() const;
    
    // 获取统计的物体数量
    int getObjectCount() const;
    
    // 设置日志级别
    void setLogLevel(LoggerLevel level);
    
    // 获取日志级别
    LoggerLevel getLogLevel();
    
    // 显示IO状态（用于诊断模式子模式1）
    void displayIOStatus();
    
    // 显示原始直径值（用于诊断模式子模式2）
    void displayRawDiameters();
};

#endif // DIAMETER_SCANNER_H