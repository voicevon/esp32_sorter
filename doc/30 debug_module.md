# DebugModule类实现要求文档

## 1. 概述

DebugModule是ESP32分拣系统的底层硬件控制模块，专门负责直接管理物理硬件接口，包括LED指示灯控制和按钮输入检测。该模块为系统提供硬件抽象层，处理所有与硬件相关的低级操作。

## 2. 技术规格

### 2.1. 硬件接口

- **LED指示灯**：2个LED（LED1和LED2）用于系统状态可视化
- **按钮**：2个按钮，用于用户输入和模式切换触发
  - 触发方式：下降沿触发中断
  - 软件去抖：50ms

## 3. 类定义与成员

### 3.1. 公共接口

- **initialize()**：初始化调试模块，配置引脚模式和初始状态
- **update()**：更新模块状态，处理按钮去抖和LED闪烁逻辑
- **setLEDState(int ledNumber, bool state)**：直接控制指定LED的开关状态
- **toggleLED(int ledNumber)**：切换指定LED的当前状态
- **isButtonPressed(int buttonNumber)**：检查指定按钮是否被按下（已实现去抖）
- **clearButtonStates()**：清除所有按钮的按下状态
- **setSystemMode(int mode)**：根据系统工作模式设置对应的LED显示模式
- **setDebounceTime(int debounceMs)**：设置按钮去抖时间
- **setBlinkInterval(int intervalMs)**：设置LED闪烁间隔

### 3.2. 静态成员

- **中断处理函数**
  - `static void handleButton1Interrupt()`：按钮1中断处理函数
  - `static void handleButton2Interrupt()`：按钮2中断处理函数

- **实例引用**
  - `static DebugModule* instance`：指向DebugModule类实例的静态指针，用于中断处理函数访问

### 3.3. 私有成员变量

- **硬件配置**
  - `int button1Pin`：按钮1连接的引脚号
  - `int button2Pin`：按钮2连接的引脚号
  - `int led1Pin`：LED1连接的引脚号
  - `int led2Pin`：LED2连接的引脚号

- **状态变量**
  - `bool button1Pressed`：按钮1按下标志
  - `bool button2Pressed`：按钮2按下标志
  - `unsigned long lastButton1Time`：按钮1最后操作时间戳
  - `unsigned long lastButton2Time`：按钮2最后操作时间戳
  - `unsigned long lastBlinkTime`：上次LED状态切换时间
  - `bool blinkState`：闪烁状态
  - `led1State`：LED1的当前状态
  - `led2State`：LED2的当前状态
  - `currentSystemMode`：当前显示的系统模式

- **配置参数**
  - `int debounceTime`：去抖延迟时间（毫秒）
  - `int blinkInterval`：LED闪烁间隔时间

## 4. LED系统状态显示

DebugModule根据系统模式提供以下LED状态显示：

- **MODE_NORMAL**：LED1灭，LED2灭
- **MODE_DIAGNOSE_ENCODER**：LED1亮，LED2灭
- **MODE_DIAGNOSE_SCANNER**：LED1灭，LED2亮
- **MODE_DIAGNOSE_DIVERTER**：LED1亮，LED2亮
- **MODE_TEST**：LED1和LED2交替闪烁（500ms间隔）
- **MODE_DIAGNOSE_CONVEYOR**：LED1慢闪，LED2灭

## 5. 按钮去抖处理

实现50ms去抖逻辑，确保可靠的按钮状态检测：

1. 监测按钮引脚状态变化，触发下降沿中断
2. 在update()方法中记录时间戳并应用去抖延迟
3. 延迟后再次读取引脚状态，确认稳定状态
4. 只有当状态持续稳定时才设置按钮按下标志
5. 通过clearButtonStates()方法清除按钮状态

## 6. LED闪烁控制

DebugModule提供LED闪烁控制功能，具体实现如下：

1. 在update()方法中根据时间间隔控制LED闪烁
2. 根据当前系统模式设置LED状态组合
3. 为MODE_TEST模式实现500ms间隔的交替闪烁
4. 提供视觉反馈，指示系统当前运行状态