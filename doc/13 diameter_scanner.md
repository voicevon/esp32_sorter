# DiameterScanner类

## 1. 概述
提供数据采样和直径计算功能。

## 2. 公共接口
- `void initialize()`：初始化引脚和缓冲区
- `void reset()`：重置状态和缓冲区
- `void sample(long encoderPosition)`：采样传感器状态
- `float getDiameter()`：获取计算的直径值

## 3. 工作原理
- 记录连续低电平采样点数
- 直径 = 连续高电平个数