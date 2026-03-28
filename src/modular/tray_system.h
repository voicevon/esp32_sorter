#ifndef TRAY_SYSTEM_H
#define TRAY_SYSTEM_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * 托盘系统类
 * 负责管理托盘数据，包括直径数据存储、扫描次数等
 * 采用单例模式实现，确保系统中只有一个实例
 */
class TraySystem {
private:
    // 常量定义
    static const uint8_t QUEUE_CAPACITY = 18; // 索引 0-17
    static const int EMPTY_TRAY = 0;  // 无效直径值，用于表示该位置没有芦笋
    
    // 成员变量
    int asparagusDiameters[QUEUE_CAPACITY];    // 存储每个芦笋的直径数据
    int asparagusCounts[QUEUE_CAPACITY];    // 存储每个位置的芦笋数量
    int asparagusLengths[QUEUE_CAPACITY];   // 存储每个芦笋的长度等级 (1:S, 2:M, 3:L)
    
    // 累计统计数据
    uint32_t totalIdentifiedItems;             // 自启动以来识别到的芦笋总数
    uint32_t totalTransportedTrays;            // 自启动以来经过的托盘总数

    // 线程安全互斥锁
    SemaphoreHandle_t mutex;
    
    // 单例实例
    static TraySystem* instance;
    
    /**
     * 将所有托盘数据向右移动（索引值+1）
     */
    void shiftToRight();
    
    /**
     * 构造函数
     * 初始化所有成员变量
     */
    TraySystem();
    
    /**
     * 析构函数
     */
    ~TraySystem();
    
    // 禁止拷贝构造和赋值操作
    TraySystem(const TraySystem&) = delete;
    TraySystem& operator=(const TraySystem&) = delete;
    
public:
    /**
     * 获取单例实例
     * @return TraySystem实例指针
     */
    static TraySystem* getInstance();
    
    /**
     * 从单一扫描仪添加新的芦笋数据（插入到索引0）
     * @param diameter 直径值
     * @param scanCount 扫描次数
     * @param lengthLevel 长度等级 (1:S, 2:M, 3:L)
     */
    void pushNewAsparagus(int diameter, int scanCount, int lengthLevel = 0);
    
    /**
     * 重置所有直径数据
     */
    void resetAllTraysData();
    
    /**
     * 获取托盘直径数据
     * @param index 托盘索引
     * @return 直径值，无效返回0
     */
    int getTrayDiameter(int index);
    
    /**
     * 获取托盘扫描次数
     * @param index 托盘索引
     * @return 扫描次数
     */
    int getTrayScanCount(int index);

    /**
     * 获取托盘长度等级
     * @param index 托盘索引
     * @return 长度等级 (1:S, 2:M, 3:L)，无效返回0
     */
    int getTrayLengthLevel(int index);
    
    /**
     * 获取托盘队列容量
     * @return 托盘队列容量
     */
    static uint8_t getCapacity();

    /**
     * 保存托盘数据到EEPROM
     * @param startAddr EEPROM起始地址
     */
    void saveToEEPROM(int startAddr);

    /**
     * 从EEPROM加载托盘数据
     * @param startAddr EEPROM起始地址
     */
    void loadFromEEPROM(int startAddr);

    /**
     * 获取逻辑自启动以来识别到的芦笋总数
     */
    uint32_t getTotalIdentifiedItems() const { return totalIdentifiedItems; }

    /**
     * 获取逻辑自启动以来经过的托盘总数
     */
    uint32_t getTransportedTrayCount() const { return totalTransportedTrays; }
};

#endif // TRAY_SYSTEM_H
