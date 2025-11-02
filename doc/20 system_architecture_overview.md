# 系统架构概述

## 1. 系统组件
- **SimpleHMI**：管理LED和按钮的硬件控制模块
- **Sorter**：核心控制类，管理CarriageSystem、Outlet和Encoder，提供spin_Once()函数
- **CarriageSystem**：管理料车数据和直径检测
- **Outlet**：控制出口舵机动作
- **Encoder**：监测传输线位置

## 2. 控制流程
- 系统初始化各组件
- 主循环在正常工作模式下调用Sorter::spin_Once()
- 处理模式切换和用户输入
- 执行相应的功能逻辑