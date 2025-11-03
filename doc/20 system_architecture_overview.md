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
- 编码器触发相位变化时自动调用Sorter::onPhaseChange()
- 根据不同相位执行相应的功能逻辑：
  - 采样相位：调用DiameterScanner采样传感器数据
  - 计算相位：计算物体直径并存储到TraySystem
  - 分拣相位：控制Outlet执行分拣动作