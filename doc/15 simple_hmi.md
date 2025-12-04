# SimpleHMI类

## 1. 概述
管理两组LED指示灯和两组按钮的硬件驱动模块，采用单例模式实现。

## 2. 公共接口
- `static SimpleHMI* getInstance()`：获取单例实例
- `void initialize()`：初始化引脚和状态
- `bool isMasterButtonPressed()`：检查主按钮是否被按下并释放（自动清除标志）
- `bool isSlaveButtonPressed()`：检查从按钮是否被按下并释放（自动清除标志）
- `void clearMasterButtonFlag()`：手动清除主按钮标志
- `void clearSlaveButtonFlag()`：手动清除从按钮标志

## 3. 核心功能
- 使用中断方式处理按钮输入
- 实现按钮50ms去抖处理
- 通过标志位表示按钮按下并释放的事件
- LED控制已移至Sorter类，仅用于显示出口状态