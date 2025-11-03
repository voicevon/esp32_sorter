#ifndef DIAMETER_SCANNER_H
#define DIAMETER_SCANNER_H

#include <Arduino.h>
#include "pins.h"

class DiameterScanner {
private:
    // 引脚定义
    int scannerPin;
    
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

public:
    // 构造函数
    DiameterScanner();
    
    // 初始化引脚和缓冲区
    void initialize();
    
    // 重置状态和缓冲区
    void reset();
    
    // 采样传感器状态（根据相位进行采样）
    void sample(int phase);
    
    // 获取计算的直径值（整数）
    int getDiameter();
    
    // 获取统计的物体数量
    int getObjectCount();
};

#endif // DIAMETER_SCANNER_H