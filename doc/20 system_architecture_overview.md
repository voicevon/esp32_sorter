# 系统架构概述

## 1. 系统组件
- **SimpleHMI**：管理LED和按钮的硬件控制模块
- **Sorter**：核心控制类，管理TraySystem、Outlet和Encoder
- **TraySystem**：管理托盘队列数据，存储每个托盘位置的直径数据和扫描次数
- **Outlet**：控制出口舵机动作，支持根据直径范围进行分拣
- **Encoder**：监测传输线位置，提供相位变化回调（0-199相位）
- **DiameterScanner**：根据编码器相位采样传感器状态，计算并返回物体直径值

## 2. 控制流程
- 系统初始化各组件
- 编码器触发相位变化时调用Sorter::onPhaseChange()，仅设置状态标志位
- main.cpp中的loop()函数周期性调用sorter.spinOnce()
- spinOnce()函数根据状态标志位执行相应的功能逻辑：
  - 重置扫描仪状态
  - 处理扫描数据并存储到TraySystem
  - 预设出口状态
  - 执行出口控制动作