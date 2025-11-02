# Outlet类

## 1. 概述
控制单个出口的舵机动作，引导物品至指定分拣通道。

## 2. 公共接口
- `Outlet()` - 构造函数
- `void initialize(int pin)` - 初始化舵机和引脚
- `void open()` - 打开出口
- `void close()` - 关闭出口
- `void test()` - 测试出口功能