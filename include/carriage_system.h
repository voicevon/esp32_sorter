#ifndef CARRIAGE_SYSTEM_H
#define CARRIAGE_SYSTEM_H

#include <Arduino.h>

// 固定出口数量
#define NUM_OUTLETS 5   // 固定安装的出口数量

// 直径分类阈值（单位：mm）
#define DIAMETER_THRESHOLD_1 15.0
#define DIAMETER_THRESHOLD_2 12.0
#define DIAMETER_THRESHOLD_3 9.0
#define DIAMETER_THRESHOLD_4 6.0

// 托架总数
#define TOTAL_CARRIAGES 31 // 索引0-30

// 无效直径值，用于表示该位置没有芦笋
#define INVALID_DIAMETER -1.0

/**
 * 传送带管理模块 - 使用简单数组存储直径数据
 */

// 直径数据数组的声明（在carriage_system.cpp中定义）
extern float carriageDiameters[TOTAL_CARRIAGES];

// 出口位置数组的声明（在carriage_system.cpp中定义）
extern uint8_t divergencePointIndices[NUM_OUTLETS];

/**
 * 初始化出口位置
 * @param positions 出口位置数组
 */
void initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]);

/**
 * 从单点扫描仪添加新的直径数据（插入到索引0）
 * @param diameter 直径值
 */
void addNewDiameterData(float diameter);

/**
 * 移动所有托架数据（索引值+1）
 */
void moveCarriagesData();

/**
 * 检查并执行分配动作
 * @param currentIndex 当前到达的索引位置
 * @return 1-5表示需要激活的舵机，0表示不需要动作
 */
uint8_t checkAndExecuteAssignment(uint8_t currentIndex);

/**
 * 重置所有直径数据
 */
void resetAllCarriagesData();

/**
 * 根据直径值确定出口
 * @param diameter 直径值
 * @return 出口编号(1-5)
 */
uint8_t determineOutlet(float diameter);

/**
 * 显示所有有效直径数据和队列状态（调试用）
 */
void displayCarriageQueue();

#endif // CARRIAGE_SYSTEM_H