# 主函数核心控制逻辑

## 1. 概述
控制ESP32分拣系统的核心逻辑，根据用户输入管理系统模式切换。

## 2. 主要组件
- **Sorter**：管理正常工作模式下的核心功能，包括直径扫描、数据存储和出口控制
- **SimpleHMI**：处理用户输入（按钮）和LED显示（模式指示）

## 3. 工作流程
- 主函数通过SimpleHMI的按钮输入管理系统模式切换
- 在MODE_NORMAL模式下，编码器触发相位变化时自动调用Sorter::onPhaseChange()
- Sorter类根据不同相位协调TraySystem、Outlet和Encoder的工作：
  - 初始化分流点位置
  - 调用DiameterScanner进行数据采样
  - 将直径数据存储到TraySystem
  - 控制相应的Outlet执行分拣动作