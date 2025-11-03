#include "tray_system.h"
#include <Arduino.h>

/**
 * 构造函数实现
 */
TraySystem::TraySystem() {
    // 初始化所有成员变量
    for (uint8_t i = 0; i < TOTAL_TRAYS; i++) {
        trayDiameters[i] = INVALID_DIAMETER;
        trayScanCount[i] = 0;
    }
}

/**
 * 添加新直径数据实现
 */
void TraySystem::addNewDiameterData(int diameter, int scanCount) {
    // 将所有现有数据向后移动一位
    moveTraysData();
    
    // 在索引0处添加新数据
    trayDiameters[0] = diameter;
    trayScanCount[0] = scanCount;
    
    // 打印添加的新数据（调试信息）
    Serial.print("添加新数据到索引0: 直径 = ");
    Serial.print(diameter);
    Serial.print(", 扫描次数 = ");
    Serial.println(scanCount);
}

/**
 * 移动托盘数据实现
 */
void TraySystem::moveTraysData() {
    // 从最后一个位置开始，向前移动数据
    trayDiameters[TOTAL_TRAYS - 1] = INVALID_DIAMETER; // 最后一个位置数据丢弃
    trayScanCount[TOTAL_TRAYS - 1] = 0; // 扫描次数重置为0
    
    for (int8_t i = TOTAL_TRAYS - 2; i >= 0; i--) {
        if (trayDiameters[i] != INVALID_DIAMETER) {
            // 将当前位置数据复制到下一个位置
            trayDiameters[i + 1] = trayDiameters[i];
            trayScanCount[i + 1] = trayScanCount[i];
        } else {
            trayDiameters[i + 1] = INVALID_DIAMETER;
            trayScanCount[i + 1] = 0;
        }
        // 重置当前位置
        trayDiameters[i] = INVALID_DIAMETER;
        trayScanCount[i] = 0;
    }
    
    Serial.println("所有直径数据已移动");
}

/**
 * 重置所有直径数据实现
 */
void TraySystem::resetAllTraysData() {
    for (uint8_t i = 0; i < TOTAL_TRAYS; i++) {
        trayDiameters[i] = INVALID_DIAMETER;
        trayScanCount[i] = 0;
    }
    Serial.println("所有直径数据和扫描次数数据已重置");
}
/**
 * 显示托盘队列实现
 */
void TraySystem::displayTrayQueue() {
    Serial.println("\n[TRAY DEBUG] ========= 队列状态详情 =========");
    Serial.println("[TRAY DEBUG] 有效直径数据列表:");
    
    bool hasValidData = false;
    
    // 显示所有有效直径数据
    for (uint8_t i = 0; i < TOTAL_TRAYS; i++) {
        if (trayDiameters[i] != INVALID_DIAMETER) {
            hasValidData = true;
            Serial.print("[TRAY DEBUG] - 位置 ");
            Serial.print(i);
            Serial.print(": 直径 = ");
            Serial.print(trayDiameters[i]);
            Serial.print(", 扫描次数 = ");
            Serial.print(trayScanCount[i]);
            Serial.println();
        }
    }
    
    if (!hasValidData) {
        Serial.println("[TRAY DEBUG] - 当前没有有效直径数据");
    }
    
    Serial.println("[TRAY DEBUG] ========= 队列状态结束 =========\n");
}

/**
 * 获取托盘直径数据实现
 */
int TraySystem::getTrayDiameter(int index) const {
    if (index >= 0 && index < TOTAL_TRAYS) {
        return trayDiameters[index];
    }
    return INVALID_DIAMETER;
}

/**
 * 获取托盘扫描次数实现
 */
int TraySystem::getTrayScanCount(int index) const {
    if (index >= 0 && index < TOTAL_TRAYS) {
        return trayScanCount[index];
    }
    return 0;
}

/**
 * 获取托盘总数实现
 */
uint8_t TraySystem::getTotalTrays() {
    return TOTAL_TRAYS;
}