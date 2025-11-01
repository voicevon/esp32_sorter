# 系统控制模块概述

## 1. 概述

ESP32分拣系统的控制核心由两个主要组件组成：
- **DebugModule**：底层硬件控制模块，负责直接管理物理硬件接口（LED、按钮）
- **DiagnosticController**：高级控制组件，负责系统工作模式管理和模块协同调度

这两个组件共同构成系统的控制层，分别处理硬件抽象和系统协调，实现简洁高效的系统控制架构。

## 2. 组件职责分配

### 2.1 DebugModule
- 管理硬件接口（2个LED、2个按钮）
- 处理按钮去抖和中断触发
- 根据系统模式控制LED状态显示
- 提供硬件抽象，简化上层调用

详细实现见：<mcfile name="30 debug_module.md" path="d:\Firmware\esp32_sorter\doc\30 debug_module.md"></mcfile>

### 2.2 DiagnosticController
- 管理系统工作模式切换（正常模式和多种诊断模式）
- 协调各功能模块的工作
- 处理用户输入和系统状态指示
- 实现各种诊断和测试功能

详细实现见：<mcfile name="31 diagnostic_controller.md" path="d:\Firmware\esp32_sorter\doc\31 diagnostic_controller.md"></mcfile>

### 2.3 SorterController
- 协调分支器控制器、托架管理器和编码器等模块
- 管理数据流转和位置跟踪
- 控制分支器动作和基于位置的复位
- 提供系统启动、停止、重置等核心功能

详细实现见：<mcfile name="25 sorter_controller.md" path="d:\Firmware\esp32_sorter\doc\25 sorter_controller.md"></mcfile>

## 3. 控制流程

系统控制的基本流程如下：
1. 系统初始化时创建并初始化DebugModule和DiagnosticController
2. 主循环中定期调用update方法更新状态
3. 处理按钮输入触发的模式切换
4. 根据当前模式执行相应的功能逻辑
5. 通过LED提供系统状态的视觉反馈

## 4. 系统模式

系统支持以下主要工作模式：
- **正常模式**：完整的分拣流程，包括编码器监测、激光扫描和分支器控制
- **诊断模式**：用于调试和测试各个子系统（编码器、扫描仪、分支器等）
- **测试模式**：综合测试，验证整个系统的协作和功能

## 5. 设计原则

- **简洁优先**：保持系统设计简洁明了，避免过度复杂化
- **职责分离**：底层硬件控制与高级逻辑控制分离
- **模块化设计**：各组件功能独立，通过明确接口交互
- **易于维护**：提供清晰的诊断机制和状态指示