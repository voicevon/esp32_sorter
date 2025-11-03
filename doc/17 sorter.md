# Sorter类

## 1. 概述
分拣系统的核心控制类，协调编码器、出口、直径扫描仪和托盘系统的工作，实现物体的扫描、数据处理和分拣功能。

## 2. 公共接口
- `Sorter()`：构造函数，初始化成员变量
- `void initialize()`：初始化分拣系统的所有组件
- `void onPhaseChange(int phase)`：采样回调函数，供编码器调用，仅设置状态标志位
- `void presetOutlets()`：根据托盘数据预设出口状态，确定哪些出口需要打开
- `void spinOnce()`：主循环执行函数，处理耗时操作，定期被main.cpp调用

## 3. 核心功能
- 协调多个子系统工作：Encoder、Outlet、DiameterScanner和TraySystem
- 管理5个固定安装的出口
- 处理编码器相位变化事件
- 初始化出口分流点位置
- 执行出口预设和分配逻辑

## 4. 内部结构
- 使用编码器单例模式获取位置信息
- 维护5个出口对象数组
- 包含直径扫描仪实例进行物体测量
- 包含托盘系统实例存储直径数据
- 记录运行状态标志
- 存储出口位置索引数组
- 包含四个volatile状态标志位：resetScannerFlag、processScanDataFlag、executeOutletsFlag和resetOutletsFlag

## 5. 回调机制
- `staticPhaseCallback`：静态回调函数，用于连接编码器相位变化事件
- 当编码器相位变化时，触发onPhaseChange方法执行相应的处理逻辑