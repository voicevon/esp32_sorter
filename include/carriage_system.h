#ifndef CARRIAGE_SYSTEM_H
#define CARRIAGE_SYSTEM_H

#include <Arduino.h>

// 分支器和舵机数量
#define NUM_DIVERTERS 5

// 直径分类阈值（单位：mm）
#define DIAMETER_THRESHOLD_1 15.0
#define DIAMETER_THRESHOLD_2 12.0
#define DIAMETER_THRESHOLD_3 9.0
#define DIAMETER_THRESHOLD_4 6.0

// 托架总数
#define TOTAL_CARRIAGES 31 // 索引0-30

/**
 * 托架类 - 表示传输线上的单个托架
 */
class Carriage {
private:
    uint8_t index;       // 索引值(0-30)
    float diameter;      // 芦笋直径值(mm)
    bool isValid;        // 是否有效
    bool isAssigned;     // 是否已分配

public:
    /**
     * 构造函数
     * @param idx 索引值
     */
    Carriage(uint8_t idx = 0);

    /**
     * 设置直径值
     * @param diam 直径值
     */
    void setDiameter(float diam);

    /**
     * 获取直径值
     * @return 直径值
     */
    float getDiameter() const;

    /**
     * 设置有效状态
     * @param valid 有效状态
     */
    void setValid(bool valid);

    /**
     * 判断是否有效
     * @return 是否有效
     */
    bool getValid() const;

    /**
     * 设置分配状态
     * @param assigned 分配状态
     */
    void setAssigned(bool assigned);

    /**
     * 判断是否已分配
     * @return 是否已分配
     */
    bool getAssigned() const;

    /**
     * 获取索引值
     * @return 索引值
     */
    uint8_t getIndex() const;

    /**
     * 重置托架状态
     */
    void reset();
};

/**
 * 托架管理器类 - 管理所有托架和分配逻辑
 */
class CarriageManager {
private:
    Carriage carriages[TOTAL_CARRIAGES];         // 托架数组
    uint8_t divergencePointIndices[NUM_DIVERTERS]; // 分支点位置数组
    bool shouldMoveCarriages;                   // 是否需要移动托架数据

public:
    /**
     * 构造函数
     */
    CarriageManager();

    /**
     * 初始化分支点位置
     * @param positions 分支点位置数组
     */
    void setDivergencePoints(const uint8_t positions[NUM_DIVERTERS]);

    /**
     * 从单点扫描仪添加新的直径数据（插入到索引0）
     * @param diameter 直径值
     */
    void addNewDiameterData(float diameter);

    /**
     * 移动所有托架数据（索引值+1）
     */
    void moveCarriages();

    /**
     * 检查并执行分配动作
     * @param currentIndex 当前到达的索引位置
     * @return 1-5表示需要激活的舵机，0表示不需要动作
     */
    uint8_t checkAndExecuteAssignment(uint8_t currentIndex);

    /**
     * 获取特定索引的托架
     * @param index 索引值
     * @return 托架引用
     */
    Carriage& getCarriage(uint8_t index);

    /**
     * 重置所有托架状态
     */
    void resetAllCarriages();

    /**
     * 根据直径值确定出口
     * @param diameter 直径值
     * @return 出口编号(1-5)
     */
    uint8_t determineOutlet(float diameter);
};

#endif // CARRIAGE_SYSTEM_H