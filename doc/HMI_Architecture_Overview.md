# HMI 交互系统架构说明

此文档描述了 ESP32 Sorter 项目中用户界面与输入系统的组织逻辑，旨在帮助您理解并优化编码器与菜单的操作体验。

## 1. 核心组件
系统由四个核心类协同工作：
- **SimpleHMI**: 底层驱动，负责读取物理引脚（编码器 A/B、按钮）的状态，并处理中断和初步事件封装（如长按/短按）。
- **MenuSystem**: 逻辑控制层，负责维护菜单树结构、当前选中的索引、滚动位置等。它不负责绘图。
- **OLED / Terminal**: 表现层，负责将 MenuSystem 的状态渲染到物理屏幕或控制台。
- **UserInterface**: 门面管理器，将上述三个类组合在一起，提供给 `main.cpp` 统一的调用接口。

## 2. 交互逻辑流 (Event Flow)

### 2.1 菜单导航
1. 旋转编码器 -> 触发 `SimpleHMI` 中断。
2. `SimpleHMI` 累计 `encoderDelta`。
3. `main.cpp` 在 `loop` 中轮询 `userInterface->getEncoderDelta()`。
4. 如果有偏移 -> 调用 `menuSystem->navigate(delta)`。
5. 调用 `userInterface->renderMenu()` 刷新屏幕。

### 2.2 动作触发 (动作选择)
1. 点击按键 -> `SimpleHMI` 记录 `masterButtonClickFlag`。
2. `main.cpp` 检测到点击 -> 获取当前选中的菜单项内容。
3. 如果是子菜单 -> 进入并刷新。
4. 如果是执行命令 (如诊断模式) -> 设置 `isMenuMode = false`，进入特定模式。

### 2.3 模式退出 (关键)
- **当前逻辑**：主循环检测到长按释放事件 (`isMasterButtonLongPressed`) -> 调用 `handleReturnToMenu()` -> 将 `isMenuMode` 设回 `true` 并重置显示。
- **优化草案**：将诊断模式内的轮询逻辑改为检测**短按事件**，以此实现快速退出。

## 3. 状态机编码器逻辑
目前的解码逻辑已升级为全状态机：
- **状态 (0, 1, 2, 3)** 代表 A/B 引脚的 `(00, 01, 10, 11)`。
- 只有合法的跳转路径（如 `10 -> 11` 代表顺时针转动了一半）才会被计数。
- 这能有效过滤由于接触不良或长线干扰造成的单向抖动。

## 4. 常见的“迟钝感”来源
- **UI 更新间隔**：`UserInterface` 默认有更新频率限制，确保 OLED 不会因为过载烧毁或占用过多 CPU。
- **模式切换弹窗**：之前显示的 "Mode Changed" 全屏信息占用了显示周期，导致实际功能画面延迟出现。目前正在彻底移除这些多余提示。
