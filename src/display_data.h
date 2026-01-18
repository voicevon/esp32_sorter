#ifndef DISPLAY_DATA_H
#define DISPLAY_DATA_H

#include <Arduino.h>
#include "main.h"

struct DisplayData {
    // 系统模式信息
    SystemMode currentMode;
    uint8_t outletCount;
    
    // 子模式信息
    int normalSubMode;       // 正常模式子模式：0=统计信息, 1=最新直径
    int encoderSubMode;      // 编码器诊断模式子模式：0=位置显示, 1=相位变化
    int outletSubMode;       // 出口诊断模式子模式：0=轮巡降落, 1=轮巡上升
    
    // 编码器信息
    int encoderPosition;
    bool encoderPositionChanged;
    
    // 分拣速度信息
    int sortingSpeedPerSecond;   // 根/秒
    int sortingSpeedPerMinute;   // 根/分钟
    int sortingSpeedPerHour;      // 根/小时
    
    // 统计信息
    int identifiedCount;      // 已识别的物体数量
    int transportedTrayCount; // 已经输送的托架数量
    
    // 直径信息
    int latestDiameter;      // 最新直径值
    
    // 出口测试模式信息
    uint8_t openOutlet;     // 当前打开的出口（255表示没有打开的出口）
    
    // 构造函数，初始化所有字段为默认值
    DisplayData() :
        currentMode(static_cast<SystemMode>(0)),
        outletCount(0),
        normalSubMode(0),
        encoderSubMode(0),
        outletSubMode(0),
        encoderPosition(0),
        encoderPositionChanged(false),
        sortingSpeedPerSecond(0),
        sortingSpeedPerMinute(0),
        sortingSpeedPerHour(0),
        identifiedCount(0),
        transportedTrayCount(0),
        latestDiameter(0),
        openOutlet(255) {}
};

#endif // DISPLAY_DATA_H
