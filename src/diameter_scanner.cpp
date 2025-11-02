#include "diameter_scanner.h"

DiameterScanner::DiameterScanner() : 
    scannerPin(LASER_SCANNER_PIN),
    highLevelCount(0),
    objectCount(0),
    lastSensorState(false),
    isSampling(false),
    calculatedDiameter(0) {
}

void DiameterScanner::initialize() {
    // 设置扫描仪引脚为输入模式
    pinMode(scannerPin, INPUT);
    
    // 初始化状态
    reset();
    
    Serial.println("DiameterScanner initialized");
}

void DiameterScanner::reset() {
    // 重置所有状态变量
    highLevelCount = 0;
    objectCount = 0;
    lastSensorState = false;
    isSampling = false;
    calculatedDiameter = 0;
}

void DiameterScanner::sample(int phase) {
    // 读取传感器状态
    bool currentState = (digitalRead(scannerPin) == HIGH);
    
    // 检测到物体（高电平）
    if (currentState) {
        // 开始或继续采样
        if (!isSampling) {
            isSampling = true;
            highLevelCount = 0;
            
            // 如果上一次是低电平（表示断开后又出现高电平），增加物体计数
            if (!lastSensorState) {
                objectCount++;
                Serial.print("[SCANNER] Object detected, count: ");
                Serial.println(objectCount);
            }
        }
        
        // 增加高电平计数
        highLevelCount++;
    }
    // 未检测到物体（低电平）
    else {
        // 结束采样并计算直径（直接使用高电平计数）
        if (isSampling) {
            calculatedDiameter = highLevelCount; // 直径 = 连续高电平个数
            
            // 重置采样状态
            isSampling = false;
            
            // 输出调试信息
            Serial.print("[SCANNER] Diameter calculated: ");
            Serial.print(calculatedDiameter);
            Serial.println(" units");
        }
    }
    
    // 保存当前状态用于下一次比较
    lastSensorState = currentState;
}

int DiameterScanner::getDiameter() {
    return calculatedDiameter;
}

int DiameterScanner::getObjectCount() {
    return objectCount;
}