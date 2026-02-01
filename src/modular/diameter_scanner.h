#ifndef DIAMETER_SCANNER_H
#define DIAMETER_SCANNER_H

#include <Arduino.h>
#include "../config.h"

#include "../utils/singleton.h"

class DiameterScanner : public Singleton<DiameterScanner> {
    friend class Singleton<DiameterScanner>;
private:
    // 单例模式静态实例 - Managed by Singleton
    // static DiameterScanner* instance;
    
    // 引脚定义（4个扫描点）
    int scannerPins[4];
    bool isScanning;
    // 高电平采样计数（每个扫描点）
    int highLevelPulseCounts[4];
    
    // 统计次数（计算实际物体数量）- 每个扫描点一个计数器
    int objectCount[4];
    
    // 上一次的传感器状态（每个扫描点）
    bool lastSensorStates[4];
    
    // 是否正在采样（每个扫描点）
    bool isObjectPassing[4];

    // 计算得到的直径值（整数）
    int nominalDiameter;
    

    // 私有构造函数，防止外部创建实例
    DiameterScanner();

public:
    // 单例模式获取实例的静态方法 - Managed by Singleton
    // static DiameterScanner* getInstance();
    
    // 初始化引脚和缓冲区
    void initialize();
    
    // 重置状态和缓冲区
    void start();

    // 检查是否正在扫描
    bool isScanningActive() const { return isScanning; }
    
    // 采样传感器状态（根据相位进行采样）- 用于直径测量
    void sample(int phase);
    
    // 获取计算的直径值（整数）并停止扫描 (Calculation moved here)
    int getDiameterAndStop();
    
    // 获取统计的物体数量
    int getObjectCount(int index) const;
    
    // 获取所有扫描点的物体数量总和
    int getTotalObjectCount() const;

    // 获取IO状态数组（用于诊断模式子模式1）
    bool* getIOStatusArray();

    // 获取单个传感器的高电平脉冲计数
    int getHighLevelPulseCount(int index) const;
    
    // 获取传感器权重
    float getSensorWeight(int index) const;
};

#endif // DIAMETER_SCANNER_H