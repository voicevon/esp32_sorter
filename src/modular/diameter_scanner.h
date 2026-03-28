#ifndef DIAMETER_SCANNER_H
#define DIAMETER_SCANNER_H

#include <Arduino.h>
#include "../config.h"
#include <atomic>

#include "../utils/singleton.h"

class DiameterScanner : public Singleton<DiameterScanner> {
    friend class Singleton<DiameterScanner>;
private:
    // 单例模式静态实例 - Managed by Singleton
    // static DiameterScanner* instance;
    
    // 引脚定义（4个扫描点）
    int scannerPins[4];
    std::atomic<bool> isScanning;
    // 高电平采样计数（每个扫描点）
    volatile int highLevelPulseCounts[4];
    
    // 统计次数（计算实际物体数量）- 每个扫描点一个计数器
    volatile int objectCount[4];
    
    // 上一次的传感器状态（每个扫描点）
    bool lastSensorStates[4];
    volatile int lastPhase; // 记录上一次处理的相位 (ISR 内部使用)
    volatile bool isObjectPassing[4];

    static const int MAX_SAMPLES = 200;
    volatile uint8_t sensorBuffers[4][MAX_SAMPLES];
    std::atomic<int> sampleCount;

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
    
    // 停止扫描计数
    void stop();

    // 检查是否正在扫描
    bool isScanningActive() const { return isScanning; }
    
    // 采样传感器状态（根据相位进行采样）- 用于直径测量
    void sample(int phase);
    
    // 获取计算的直径值（整数）并停止扫描 (Calculation moved here)
    int getDiameterAndStop();
    
    // 获取统计的物体数量
    int getObjectCount(int index) const;
    
    // 获取长度级别 (1:S, 2:M, 3:L)
    int getLengthLevel();
    
    // 获取所有扫描点的物体数量总和
    int getTotalObjectCount() const;

    // 获取缓冲区的总采样数
    int getSampleCount() const { return sampleCount; }
    
    // 获取特定扫描点在特定采样阶段的状态
    uint8_t getSample(int sensorIndex, int sampleIndex) const {
        if (sensorIndex >= 0 && sensorIndex < 4 && sampleIndex >= 0 && sampleIndex < sampleCount) {
            return sensorBuffers[sensorIndex][sampleIndex];
        }
        return 0;
    }

    // 获取IO状态数组（用于诊断模式子模式1）
    bool* getIOStatusArray();

    // 获取单个传感器的高电平脉冲计数
    int getHighLevelPulseCount(int index) const;
    
    // 获取传感器权重
    float getSensorWeight(int index) const;
};

#endif // DIAMETER_SCANNER_H