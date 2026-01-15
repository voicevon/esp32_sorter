# Encoder类

## 1. 概述
Encoder类提供位置跟踪和中断处理，采用单例模式实现。相位范围0-199，实现编码器计数值的精确跟踪。

## 2. 公共接口
- `static Encoder* getInstance()`：获取单例实例
- `void initialize()`：初始化编码器引脚和中断
- `int getCurrentPosition()`：获取当前逻辑位置（0-199）
- `void setPhaseCallback(void* context, PhaseCallback callback)`：设置相位回调函数和上下文
- `bool hasPositionChanged() const`：检查位置是否变化
- `void resetPositionChanged()`：重置位置变化标志
- `void printout()`：打印调试信息

## 3. 中断处理
- A相和B相中断：使用双中断模式和四状态解码算法，精确跟踪旋转方向和计数值
- Z相中断：在零位位置重置计数值

## 4. 回调机制
- 相位回调：当编码器相位变化时触发，参数包括回调上下文和当前相位值

## 5. 核心功能
- 提供0-199相位位置信息
- 使用AB相双中断模式提高计数精度和抗干扰能力
- 单例模式确保全局唯一实例