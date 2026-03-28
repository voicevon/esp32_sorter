#include "tray_system.h"
#include <Arduino.h>
#include <EEPROM.h>

// 初始化静态实例变量
TraySystem* TraySystem::instance = nullptr;

/**
 * 构造函数实现
 */
TraySystem::TraySystem() {
    // 1. 创建互斥锁
    mutex = xSemaphoreCreateMutex();
    
    // 2. 初始化所有成员变量
    for (uint8_t i = 0; i < QUEUE_CAPACITY; i++) {
        asparagusDiameters[i] = EMPTY_TRAY;
        asparagusCounts[i] = 0;
        asparagusLengths[i] = 0;
    }
    totalIdentifiedItems = 0;
    totalTransportedTrays = 0;
    Serial.println("[TRAY] TraySystem instance created (Thread-safe).");
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
void TraySystem::pushNewAsparagus(int diameter, int scanCount, int lengthLevel) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // 将所有现有数据向右移动一位
        shiftToRight();
        
        // 在索引0处添加新数据
        asparagusDiameters[0] = diameter;
        asparagusCounts[0] = scanCount;
        asparagusLengths[0] = lengthLevel;
        
        // 映射：只有直径大于 6mm 的芦笋才算作一个有效 Item (每个托盘最多计 1 个)
        if (diameter > 6 && scanCount > 0) {
            totalIdentifiedItems++;
        }
        totalTransportedTrays++;

        xSemaphoreGive(mutex);
    } else {
        Serial.println("[TRAY] Warning: Failed to get mutex in pushNewAsparagus");
    }
}

/**
 * 移动托盘数据实现
 */
void TraySystem::shiftToRight() {
    // 从最后一个位置开始，向前移动数据
    asparagusDiameters[QUEUE_CAPACITY - 1] = EMPTY_TRAY; // 最后一个位置数据丢弃
    asparagusCounts[QUEUE_CAPACITY - 1] = 0; // 扫描次数重置为0
    asparagusLengths[QUEUE_CAPACITY - 1] = 0;
    
    for (int8_t i = QUEUE_CAPACITY - 2; i >= 0; i--) {
        if (asparagusDiameters[i] != EMPTY_TRAY) {
            // 将当前位置数据复制到下一个位置
            asparagusDiameters[i + 1] = asparagusDiameters[i];
            asparagusCounts[i + 1] = asparagusCounts[i];
            asparagusLengths[i + 1] = asparagusLengths[i];
        } else {
            asparagusDiameters[i + 1] = EMPTY_TRAY;
            asparagusCounts[i + 1] = 0;
            asparagusLengths[i + 1] = 0;
        }
        // 重置当前位置
        asparagusDiameters[i] = EMPTY_TRAY;
        asparagusCounts[i] = 0;
        asparagusLengths[i] = 0;
    }
    
    // Serial.println("所有直径数据已向右移动");
}

/**
 * 重置所有直径数据实现
 */
void TraySystem::resetAllTraysData() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        for (uint8_t i = 0; i < QUEUE_CAPACITY; i++) {
            asparagusDiameters[i] = EMPTY_TRAY;
            asparagusCounts[i] = 0;
            asparagusLengths[i] = 0;
        }
        xSemaphoreGive(mutex);
        Serial.println("所有分拣数据已重置");
    }
}


/**
 * 获取托盘直径数据实现
 */
int TraySystem::getTrayDiameter(int index) {
    int val = EMPTY_TRAY;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        if (index >= 0 && index < QUEUE_CAPACITY) {
            val = asparagusDiameters[index];
        }
        xSemaphoreGive(mutex);
    }
    return val;
}

/**
 * 获取托盘扫描次数实现
 */
int TraySystem::getTrayScanCount(int index) {
    int val = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        if (index >= 0 && index < QUEUE_CAPACITY) {
            val = asparagusCounts[index];
        }
        xSemaphoreGive(mutex);
    }
    return val;
}

/**
 * 获取托盘长度等级实现
 */
int TraySystem::getTrayLengthLevel(int index) {
    int val = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        if (index >= 0 && index < QUEUE_CAPACITY) {
            val = asparagusLengths[index];
        }
        xSemaphoreGive(mutex);
    }
    return val;
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
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        int addr = startAddr;
        EEPROM.write(addr++, 0xCC);
        for (uint8_t i = 0; i < QUEUE_CAPACITY; i++) {
            EEPROM.put(addr, asparagusDiameters[i]);
            addr += sizeof(int);
            EEPROM.put(addr, asparagusCounts[i]);
            addr += sizeof(int);
        }
        xSemaphoreGive(mutex);
    }
}

/**
 * 从EEPROM加载托盘数据实现
 */
void TraySystem::loadFromEEPROM(int startAddr) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        int addr = startAddr;
        uint8_t magic = EEPROM.read(addr++);
        if (magic == 0xCC) {
            for (uint8_t i = 0; i < QUEUE_CAPACITY; i++) {
                EEPROM.get(addr, asparagusDiameters[i]);
                addr += sizeof(int);
                EEPROM.get(addr, asparagusCounts[i]);
                addr += sizeof(int);
            }
            Serial.println("[TRAY] Tray data restored from EEPROM.");
        } else {
            Serial.println("[TRAY] No valid tray data in EEPROM.");
        }
        xSemaphoreGive(mutex);
    }
}
