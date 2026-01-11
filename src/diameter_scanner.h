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
    // 引脚定义
    int scannerPin;
    bool isScanning;
    // 高电平采样计数
    int highLevelCount;
    
    // 统计次数（计算实际物体数量）
    int objectCount;
    
    // 上一次的传感器状态
    bool lastSensorState;
    
    // 是否正在采样
    bool isSampling;
    
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
};

#endif // DIAMETER_SCANNER_H