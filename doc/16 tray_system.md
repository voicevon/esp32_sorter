# TraySystem类

## 1. 概述
管理托盘队列数据，存储每个托盘位置的直径数据和扫描次数。系统共有31个托盘位置（索引0-30）。

## 2. 公共接口
- `TraySystem()`：构造函数，初始化所有成员变量
- `void addNewDiameterData(int diameter, int scanCount)`：从单点扫描仪添加新的直径数据（插入到索引0）
- `void moveTraysData()`：移动所有托盘数据（索引值+1）
- `void resetAllTraysData()`：重置所有直径数据
- `void displayTrayQueue()`：显示所有有效直径数据和队列状态（调试用）
- `int getTrayDiameter(int index) const`：获取托盘直径数据
- `int getTrayScanCount(int index) const`：获取托盘扫描次数
- `static uint8_t getTotalTrays()`：获取托盘总数

## 3. 核心功能
- 维护31个托盘的数据队列
- 无效直径值为0，表示该位置没有物体
- 支持数据的添加、移动、重置和查询
- 提供调试显示功能