#include "tray_system.h"
#include <Arduino.h>
#include <EEPROM.h>

// 初始化静态实例变量
TraySystem* TraySystem::instance = nullptr;

/**
 * 构造函数实现
 */
TraySystem::TraySystem() {
    // 初始化所有成员变量
    for (uint8_t i = 0; i < QUEUE_CAPACITY; i++) {
        asparagusDiameters[i] = EMPTY_TRAY;
        asparagusCounts[i] = 0;
    }
    Serial.println("[TRAY] TraySystem instance created");
}

/**
 * 析构函数实现
 */
TraySystem::~TraySystem() {
    Serial.println("[TRAY] TraySystem instance destroyed");
}

/**
 * 获取单例实例实现
 */
TraySystem* TraySystem::getInstance() {
    if (instance == nullptr) {
        instance = new TraySystem();
    }
    return instance;
}

/**
 * 添加新直径数据实现
 */
void TraySystem::pushNewAsparagus(int diameter, int scanCount) {
    // 将所有现有数据向右移动一位
    shiftToRight();
    
    // 在索引0处添加新数据
    asparagusDiameters[0] = diameter;
    asparagusCounts[0] = scanCount;
    
    // 注释掉调试日志，根据需求不显示添加数据的信息
    // Serial.print("添加新数据到索引0: 直径 = ");
    // Serial.print(float(diameter)/2.0);
    // Serial.print("mm, 扫描次数 = ");
    // Serial.println(scanCount);
}

/**
 * 移动托盘数据实现
 */
void TraySystem::shiftToRight() {
    // 从最后一个位置开始，向前移动数据
    asparagusDiameters[QUEUE_CAPACITY - 1] = EMPTY_TRAY; // 最后一个位置数据丢弃
    asparagusCounts[QUEUE_CAPACITY - 1] = 0; // 扫描次数重置为0
    
    for (int8_t i = QUEUE_CAPACITY - 2; i >= 0; i--) {
        if (asparagusDiameters[i] != EMPTY_TRAY) {
            // 将当前位置数据复制到下一个位置
            asparagusDiameters[i + 1] = asparagusDiameters[i];
            asparagusCounts[i + 1] = asparagusCounts[i];
        } else {
            asparagusDiameters[i + 1] = EMPTY_TRAY;
            asparagusCounts[i + 1] = 0;
        }
        // 重置当前位置
        asparagusDiameters[i] = EMPTY_TRAY;
        asparagusCounts[i] = 0;
    }
    
    // Serial.println("所有直径数据已向右移动");
}

/**
 * 重置所有直径数据实现
 */
void TraySystem::resetAllTraysData() {
    for (uint8_t i = 0; i < QUEUE_CAPACITY; i++) {
        asparagusDiameters[i] = EMPTY_TRAY;
        asparagusCounts[i] = 0;
    }
    Serial.println("所有直径数据和扫描次数数据已重置");
}


/**
 * 获取托盘直径数据实现
 */
int TraySystem::getTrayDiameter(int index) const {
    if (index >= 0 && index < QUEUE_CAPACITY) {
        return asparagusDiameters[index];
    }
    return EMPTY_TRAY;
}

/**
 * 获取托盘扫描次数实现
 */
int TraySystem::getTrayScanCount(int index) const {
    if (index >= 0 && index < QUEUE_CAPACITY) {
        return asparagusCounts[index];
    }
    return 0;
}

/**
 * 获取托盘总数实现
 */
uint8_t TraySystem::getCapacity() {
    return QUEUE_CAPACITY;
}

/**
 * 保存托盘数据到EEPROM实现
 */
void TraySystem::saveToEEPROM(int startAddr) {
    int addr = startAddr;
    
    // 保存魔术值以验证数据有效性
    EEPROM.write(addr++, 0xCC); // Magic Byte for Tray Data
    
    // 保存数组数据
    for (uint8_t i = 0; i < QUEUE_CAPACITY; i++) {
        EEPROM.put(addr, asparagusDiameters[i]);
        addr += sizeof(int);
        EEPROM.put(addr, asparagusCounts[i]);
        addr += sizeof(int);
    }
    // 注意：调用者负责commit
}

/**
 * 从EEPROM加载托盘数据实现
 */
void TraySystem::loadFromEEPROM(int startAddr) {
    int addr = startAddr;
    
    // 检查魔术值
    uint8_t magic = EEPROM.read(addr++);
    if (magic != 0xCC) {
        Serial.println("[TRAY] No valid tray data in EEPROM found.");
        resetAllTraysData();
        return;
    }
    
    // 加载数组数据
    for (uint8_t i = 0; i < QUEUE_CAPACITY; i++) {
        EEPROM.get(addr, asparagusDiameters[i]);
        addr += sizeof(int);
        EEPROM.get(addr, asparagusCounts[i]);
        addr += sizeof(int);
    }
    Serial.println("[TRAY] Tray data restored from EEPROM.");
}
