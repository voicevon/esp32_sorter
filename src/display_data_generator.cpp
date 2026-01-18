#include "display_data_generator.h"
#include "encoder.h"

DisplayDataGenerator::DisplayDataGenerator(AsparagusTrayManager& traySystem) :
    scanner(NULL),
    traySystem(traySystem),
    lastSpeedCheckTime(0),
    lastObjectCount(0) {
}

void DisplayDataGenerator::initialize() {
    // 获取编码器实例
    encoder = Encoder::getInstance();
    
    // 获取直径扫描仪单例实例
    if (!scanner) {
        scanner = DiameterScanner::getInstance();
    }
    
    // 初始化速度计算相关变量
    lastSpeedCheckTime = 0;
    lastObjectCount = 0;
}

DisplayData DisplayDataGenerator::getDisplayData(SystemMode currentMode, int normalSubMode, int encoderSubMode, int outletSubMode) {
    DisplayData data;
    
    // 设置系统模式信息
    data.currentMode = currentMode;
    data.outletCount = 10;  // 固定出口数量
    
    // 设置子模式信息
    data.normalSubMode = normalSubMode;
    data.encoderSubMode = encoderSubMode;
    data.outletSubMode = outletSubMode;
    
    // 设置编码器信息
    data.encoderPosition = encoder->getCurrentPosition();
    data.encoderPositionChanged = encoder->hasPositionChanged();
    
    // 设置分拣速度信息
    float speedPerSecond = getSortingSpeedPerSecond();
    data.sortingSpeedPerSecond = speedPerSecond;
    data.sortingSpeedPerMinute = speedPerSecond * 60.0f;
    data.sortingSpeedPerHour = speedPerSecond * 3600.0f;
    
    // 设置统计信息
    data.identifiedCount = scanner->getTotalObjectCount();
    data.transportedTrayCount = getTransportedTrayCount();
    
    // 设置直径信息
    data.latestDiameter = getLatestDiameter();
    
    // 设置出口测试模式信息（默认没有打开的出口）
    data.openOutlet = 255;
    
    return data;
}

int DisplayDataGenerator::getLatestDiameter() const {
    return traySystem.getTrayDiameter(0);
}

int DisplayDataGenerator::getTransportedTrayCount() const {
    // 假设每40个编码器脉冲对应一个托架移动
    const int pulsesPerTray = 40;
    int encoderPosition = encoder->getCurrentPosition();
    return encoderPosition / pulsesPerTray;
}

float DisplayDataGenerator::getSortingSpeedPerSecond() {
    unsigned long currentTime = millis();
    int currentCount = scanner->getTotalObjectCount();
    int countDiff = currentCount - lastObjectCount;
    int timeDiff = currentTime - lastSpeedCheckTime;
    
    // 计算每秒的速度
    float speed = 0.0f;
    if (timeDiff > 0) {
        speed = (float)(countDiff * 1000) / (float)timeDiff;
    }
    
    // 更新时间和计数
    if (timeDiff > 1000) { // 每秒更新一次
        lastSpeedCheckTime = currentTime;
        lastObjectCount = currentCount;
    }
    
    return speed;
}
