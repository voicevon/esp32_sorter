# Outlet类

## 1. 概述
控制单个出口的舵机动作，引导物品至指定分拣通道，并支持直径范围配置。

## 2. 公共接口
- `Outlet()` - 构造函数
- `void initialize(int servoPin, int minD = 0, int maxD = 0)` - 初始化舵机、引脚和直径范围（minD和maxD定义该出口可分拣的直径范围，单位为毫米）
- `void preOpen(bool state)` - 设置出口预开状态（true：准备打开出口，false：准备关闭出口）
- `void execute()` - 执行出口控制动作（根据preOpen设置的状态驱动舵机到相应位置）
- `int getMinDiameter() const` - 获取该出口可分拣的最小直径值（单位为毫米）
- `int getMaxDiameter() const` - 获取该出口可分拣的最大直径值（单位为毫米）

**注**：直径范围参数以毫米为单位，与Sorter类中使用的转换后直径值保持一致。