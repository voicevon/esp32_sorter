# DiameterScanner类

## 1. 概述
提供数据采样和直径计算功能，用于测量物体直径。

## 2. 公共接口
- `DiameterScanner()`：构造函数，初始化成员变量
- `void initialize()`：初始化引脚和状态变量
- `void start()`：重置状态变量和计数
- `void sample(int phase)`：根据编码器相位采样传感器状态
- `int ending_getDiameter()`：获取计算的直径值（单位为unit）
- `int getObjectCount()`：获取统计的物体数量

## 3. 核心功能
- 维护采样状态和物体计数
- 跟踪传感器状态变化，统计物体通过情况
- 计算物体直径值（单位为unit）

## 4. 工作原理
- 通过记录传感器高电平持续时间来计算物体直径
- 当传感器状态变化时更新采样计数和物体计数
- 使用编码器相位信息进行精确采样
- 输出直径值单位为"unit"，实际毫米数 = unit值 / 2（取整数）