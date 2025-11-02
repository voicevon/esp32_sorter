# Encoder类

## 1. 概述
Encoder类提供位置跟踪、回调事件机制和中断处理。相位范围0-199，系统单向工作。

## 2. 公共接口
- `void initialize()`：初始化编码器引脚和中断
- `void registerTickCallback(void (*callback)())`：注册相位变化回调
- `void unregisterTickCallback()`：注销相位变化回调
- `void registerPositionCallback(int position, void (*callback)())`：注册特定位置回调
- `void unregisterPositionCallback(int position)`：注销特定位置回调
- `void unregisterAllCallbacks()`：注销所有回调
- `bool isReverseRotation()`：检查反向旋转
- `unsigned long getLastInterruptTime()`：获取最后中断时间戳

## 3. 中断处理
- A相中断：更新计数值，计算相位位置
- Z相中断：标记零位位置

## 4. 回调机制
- 计数值变化回调：用于扫描仪采样
- 位置回调：在特定相位（如85）触发出口动作

## 5. 核心功能
- 提供0-199相位位置信息
- 检测反向旋转（报警状态）
- 在85相位触发出口动作