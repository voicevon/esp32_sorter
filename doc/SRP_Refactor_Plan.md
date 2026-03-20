# 职责单一原则 (SRP) 代码审查与重构计划

## 1. 现状审查与违规识别

| 模块 | 当前职责 (混合) | 违规描述 | 建议重构方案 |
| :--- | :--- | :--- | :--- |
| **ModbusController** | 1. 协议封装 (CRC/串口) <br> 2. 伺服业务逻辑 (`syncParameters`) | 低层通讯协议类不应感知“伺服电机”或其配置逻辑。 | 移除 `syncParameters` 等高层方法，仅保留寄存器读写原子操作。 |
| **SystemManager** | 1. 系统模式切换 <br> 2. 分拣参数持久化 (EEPROM 写入) | 模式管理器不应直接操作 EEPROM 来保存特定的业务配置（如直径范围）。 | 将参数保存逻辑移回 `Sorter` 或专门的配置持久化类。 |
| **Main.cpp** | 1. 系统生命周期 <br> 2. 电位器转换与指令下发 | `main.cpp` 不应包含具体的传感器到执行器的物理转换计算逻辑。 | 将电位器逻辑封装为 `PotentiometerHandler` 或集成入 `ServoManager`。 |

## 2. 逐步重构路线图

### 第一步：清理 `ModbusController` (低层去业务化)
- 移除 `syncParameters`, `setSpeed`, `setEnable`, `softReset`。
- 将这些方法中的逻辑（如果仍然需要）整合进 `ServoManager` 的非阻塞状态机中。

### 第二步：重构 `SystemManager` (解耦持久化)
- 将 `handleModeChange` 中关于 `MODE_CONFIG_DIAMETER` 的 EEPROM 写入代码移入 `Sorter::saveConfig()`。
- 使 `SystemManager` 仅关注“跳转”动作，而不关注“数据落地”。

### 第三步：瘦身 `Main.cpp` (提取输入逻辑)
- 提炼电位器滤波与映射逻辑。
- 清理 `MODE_NORMAL` 下的杂乱代码，使其调用 `activeHandler` 或 `sorter.run()`。

---
**目标**: 确保每个类“仅有一个引起它变化的原因”。
