#ifndef TRAY_SYSTEM_H
#define TRAY_SYSTEM_H

#include <Arduino.h>

/**
 * 托盘系统类
 * 负责管理托盘数据，包括直径数据存储、扫描次数等
 */
class TraySystem {
private:
    // 常量定义
    static const uint8_t TOTAL_TRAYS = 31; // 索引0-30
    static const int INVALID_DIAMETER = 0;  // 无效直径值，用于表示该位置没有芦笋
    
    // 成员变量
    int trayDiameters[TOTAL_TRAYS];    // 存储每个托盘的直径数据
    int trayScanCount[TOTAL_TRAYS];    // 存储每个托盘的扫描次数
    
public:
    /**
     * 构造函数
     * 初始化所有成员变量
     */
    TraySystem();
    
    /**
     * 从单点扫描仪添加新的直径数据（插入到索引0）
     * @param diameter 直径值
     * @param scanCount 扫描次数（整数）
     */
    void addNewDiameterData(int diameter, int scanCount);
    
    /**
     * 移动所有托盘数据（索引值+1）
     */
    void moveTraysData();
    
    /**
     * 重置所有直径数据
     */
    void resetAllTraysData();
    
    /**
     * 显示所有有效直径数据和队列状态（调试用）
     */
    void displayTrayQueue();
    
    /**
     * 设置托盘直径数据
     * @param index 托盘索引
     * @param diameter 直径值
     */
    void setTrayDiameter(int index, int diameter);
    
    /**
     * 设置托盘扫描次数
     * @param index 托盘索引
     * @param scanCount 扫描次数
     */
    void setTrayScanCount(int index, int scanCount);
    
    /**
     * 获取托盘直径数据
     * @param index 托盘索引
     * @return 直径值
     */
    int getTrayDiameter(int index) const;
    
    /**
     * 获取托盘扫描次数
     * @param index 托盘索引
     * @return 扫描次数
     */
    int getTrayScanCount(int index) const;
    
    /**
     * 检查托盘是否有有效数据
     * @param index 托盘索引
     * @return 是否有有效数据
     */
    bool isTrayValid(int index) const;
    
    /**
     * 获取总托盘数量
     * @return 总托盘数量
     */
    static uint8_t getTotalTrays();
    

};

#endif // TRAY_SYSTEM_H