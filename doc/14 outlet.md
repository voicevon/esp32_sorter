# Outlet类

## 1. 概述
控制单个出口的舵机动作，引导物品至指定分拣通道，并支持直径范围配置。

## 2. 公共接口
- `Outlet()` - 构造函数
- `void initialize(int servoPin, int minD = 0, int maxD = 0)` - 初始化舵机、引脚和直径范围（minD和maxD定义该出口可分拣的直径范围）
- `void PreOpen(bool state)` - 设置出口预开状态（true：准备打开出口，false：准备关闭出口）
- `void execute()` - 执行出口控制动作（根据PreOpen设置的状态驱动舵机到相应位置）
- `int getMinDiameter() const` - 获取该出口可分拣的最小直径值
- `int getMaxDiameter() const` - 获取该出口可分拣的最大直径值