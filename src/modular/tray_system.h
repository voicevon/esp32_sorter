#ifndef TRAY_SYSTEM_H
#define TRAY_SYSTEM_H

#include <Arduino.h>

/**
 * 托盘系统类
 * 负责管理托盘数据，包括直径数据存储、扫描次数等
 * 采用单例模式实现，确保系统中只有一个实例
 */
class TraySystem {
private:
    // 常量定义
    static const uint8_t QUEUE_CAPACITY = 19; // 索引0-18
    static const int EMPTY_TRAY = 0;  // 无效直径值，用于表示该位置没有芦笋
    
    // 成员变量
    int asparagusDiameters[QUEUE_CAPACITY];    // 存储每个芦笋的直径数据
    int asparagusCounts[QUEUE_CAPACITY];    // 存储每个位置的芦笋数量
    
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
     * 从单点扫描仪添加新的芦笋数据（插入到索引0）
     * @param diameter 直径值
     * @param scanCount 扫描次数（整数）
     */
    void pushNewAsparagus(int diameter, int scanCount);
    
    /**
     * 重置所有直径数据
     */
    void resetAllTraysData();
    
    /**
     * 获取托盘直径数据
     * @param index 托盘索引
     * @return 直径值，无效返回0
     */
    int getTrayDiameter(int index) const;
    
    /**
     * 获取托盘扫描次数
     * @param index 托盘索引
     * @return 扫描次数
     */
    int getTrayScanCount(int index) const;
    
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

};

#endif // TRAY_SYSTEM_H
