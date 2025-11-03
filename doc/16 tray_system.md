# TraySystem类

## 1. 概述
管理托盘数据，包括直径数据存储、扫描次数等。

## 2. 公共接口
- `void addNewDiameterData(int diameter, int scanCount)`：从单点扫描仪添加新的直径数据（插入到索引0）
- `void moveTraysData()`：移动所有托盘数据（索引值+1）
- `void resetAllTraysData()`：重置所有直径数据
- `void displayTrayQueue()`：显示所有有效直径数据和队列状态（调试用）
- `void setTrayDiameter(int index, int diameter)`：设置托盘直径数据
- `void setTrayScanCount(int index, int scanCount)`：设置托盘扫描次数
- `int getTrayDiameter(int index) const`：获取托盘直径数据
- `int getTrayScanCount(int index) const`：获取托盘扫描次数
- `bool isTrayValid(int index) const`：检查托盘是否有有效数据
- `static uint8_t getTotalTrays()`：获取总托盘数量
- `static int getInvalidDiameter()`：获取无效直径值