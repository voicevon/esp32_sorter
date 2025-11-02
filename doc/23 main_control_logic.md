# 主函数核心控制逻辑

## 1. 概述
控制ESP32分拣系统的核心逻辑，管理系统模式并在正常模式下调用Sorter类。

## 2. 主要组件
- **Sorter**：管理正常工作模式下的核心功能，提供spin_Once()函数
- **SimpleHMI**：处理用户输入和LED显示

## 3. 工作流程
- 主函数管理系统模式切换
- 在正常工作模式下，主循环调用Sorter::spin_Once()
- Sorter类负责协调TraySystem、Outlet和Encoder的工作