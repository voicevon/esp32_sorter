#ifndef TRAY_MANAGER_H
#define TRAY_MANAGER_H

#include <Arduino.h>

/**
 * 托盘管理类
 * 负责管理托盘数据，包括直径数据存储、扫描次数等
 */
class TrayManager {
private:
    // 常量定义
    static const uint8_t QUEUE_CAPACITY = 31; // 索引0-30
    static const int EMPTY_TRAY = 0;  // 无效直径值，用于表示该位置没有芦笋
    
    // 成员变量
    int asparagusDiameters[QUEUE_CAPACITY];    // 存储每个芦笋的直径数据
    int asparagusCounts[QUEUE_CAPACITY];    // 存储每个位置的芦笋数量
    
    /**
     * 将所有托盘数据向右移动（索引值+1）
     */
    void shiftToRight();
    
public:
    /**
     * 构造函数
     * 初始化所有成员变量
     */
    TrayManager();
    
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

};

#endif // TRAY_MANAGER_H