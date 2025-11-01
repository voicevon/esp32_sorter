# DiagnosticController类实现要求文档

## 1. 概述

DiagnosticController类是ESP32分拣系统的核心控制组件，负责管理系统工作模式切换和调度各模块协同工作。该类与DebugModule类协作，DebugModule负责底层硬件（按钮和LED）的直接控制，而DiagnosticController负责高级模式管理和系统协调。

## 2. 功能概述

DiagnosticController提供以下核心功能：

- 系统工作模式管理和切换
- 与DebugModule协作处理按钮输入和LED状态指示
- 各种工作模式的实现和调度
- 诊断信息的串口输出和系统状态监控

## 3. 工作模式定义

诊断控制器管理以下系统工作模式：

- **MODE_NORMAL**: 正常工作模式，执行完整的分拣流程，通过编码器监测运行状态，使用激光扫描仪检测物品直径，并控制分支器进行分拣。

- **MODE_DIAGNOSE_ENCODER**: 编码器诊断模式，模拟移动传输线（每1.5秒移动一个位置），显示移动后的位置信息，验证编码器计数和位置跟踪功能。

- **MODE_DIAGNOSE_SCANNER**: 扫描仪诊断模式，每2秒生成随机直径数据（3mm-20mm），显示直径数据、分类阈值和预期出口，并展示队列状态。

- **MODE_DIAGNOSE_DIVERTER**: 分支器诊断模式，用于分支器系统测试的框架。

- **MODE_TEST**: 系统测试模式，综合测试模式，每3秒生成随机直径数据，每1秒模拟移动传输线，测试完整的分拣流程。

## 4. 模式切换机制

诊断控制器使用以下方式进行模式切换：

- 通过物理按钮触发模式切换（带50ms去抖处理）
- 使用内部状态机管理模式切换流程
- 模式切换时设置待切换标志，在主循环中应用模式切换
- 模式切换时通过LED指示灯和串口输出提供视觉和文本反馈

## 5. 与DebugModule的协作

DiagnosticController通过以下方式与DebugModule协作：

- 使用DebugModule提供的按钮检测接口监控按钮状态
- 调用DebugModule的LED控制方法显示当前系统模式
- 将系统模式信息传递给DebugModule，由其处理具体的LED状态显示逻辑
- 利用DebugModule的中断处理机制捕获按钮输入事件

## 6. 类接口

DiagnosticController提供以下公共接口：

- **initialize(DebugModule& debugModule)**：初始化诊断控制器，接收DebugModule引用
- **update()**：更新诊断控制器状态，检查模式切换需求
- **processCurrentMode(SorterController& sorterController)**：处理当前工作模式的逻辑
- **getCurrentMode()**：获取当前系统工作模式
- **isModeChangePending()**：检查是否有模式切换待处理
- **applyModeChange()**：应用待切换的工作模式并通知DebugModule更新LED显示
- **switchModeExternally()**：通过外部命令切换到下一个模式
- **getCurrentModeName()**：获取当前模式的字符串描述

## 7. 使用方法

### 7.1 初始化

在系统初始化阶段，创建诊断控制器实例并调用initialize()方法：

```cpp
DiagnosticController diagnosticController;
diagnosticController.initialize();
```

### 7.2 主循环中使用

在系统主循环中，需要定期调用诊断控制器的方法：

```cpp
// 更新诊断器状态
diagnosticController.update();

// 检查并应用模式切换
if (diagnosticController.isModeChangePending()) {
  diagnosticController.applyModeChange();
}

// 处理当前工作模式
diagnosticController.processCurrentMode(sorterController);
```

### 7.3 获取当前状态

可以随时获取当前系统模式和状态信息：

```cpp
// 获取当前工作模式
SystemMode currentMode = diagnosticController.getCurrentMode();

// 获取当前模式名称
String modeName = diagnosticController.getCurrentModeName();
```

## 8. 注意事项

- 诊断控制器的update()方法应该在主循环中频繁调用，以确保按钮输入得到及时响应
- 模式切换后，相关的状态变量会被重置，以确保新模式下的正确行为
- 串口调试信息在不同工作模式下会提供不同级别的详细信息
- 所有模式处理逻辑都封装在诊断控制器内部，主程序只需调用processCurrentMode方法