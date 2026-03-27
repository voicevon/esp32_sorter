# ESP32 Sorter 重构计划

## 背景

目前项目可以正常工作，重构目标是**提升可维护性和可靠性**，而非功能性变更。以下按优先级排列。

---

## R1. 消除 EEPROM 魔法地址分裂 ⭐⭐⭐

### 现状问题

EEPROM 地址分散在两处，且 `sorter.cpp` 内部使用裸常量，与 `config.h` 中的集中定义脱节。

| 位置 | 问题 |
|---|---|
| `sorter.cpp:64` | `const int EEPROM_DIAMETER_RANGES_ADDR = 0;` 局部常量，与 `config.h: EEPROM_ADDR_DIAMETER=0` 重复 |
| `sorter.cpp:77` | `EEPROM.read(addr + 25)` — 魔法偏移，出口数量变化时会默默地读错地址 |
| `sorter.cpp:424` | `EEPROM.begin(512)` 重复调用，`main.cpp` 已经 `begin()` 过 |
| `config.h` | 地址布局无注释说明占用字节数，只有起始地址 |

### 建议改法

1. 删除 `sorter.cpp` 内的局部常量，改用 `config.h` 的 `EEPROM_ADDR_DIAMETER`
2. 将偏移 `1 + i*3` 和 `+ 25` 改为命名常量：
   ```cpp
   constexpr int EEPROM_ADDR_DIAMETER = 0;        // 1 byte: magic marker 0xAA
   constexpr int EEPROM_ADDR_DIAMETER_DATA = 1;   // NUM_OUTLETS * 3 bytes
   constexpr int EEPROM_ADDR_OUTLET0_MODE = 25;   // 1 byte
   ```
3. 删除 `sorter.cpp` 中的 `EEPROM.begin(512)` 重复调用
4. 在 `config.h` 中为所有地址补上占用字节注释（已有 `PHASE_OFFSET`，统一风格）

**风险：低。纯常量替换，行为不变。**

---

## R2. Sorter 类职责过重 ⭐⭐

### 现状问题

`Sorter`（438行）目前包含 5 项不同职责：

| 职责 | 相关方法 |
|---|---|
| 分拣决策 | `prepareOutlets()`, `run()` |
| EEPROM 读写 | `restoreOutletConfig()`, `saveConfig()` |
| 硬件输出驱动 | `updateShiftRegisters()` |
| 速度计算 | `getConveyorSpeedPerSecond()` |
| 出口暴露（给诊断模式用） | `getOutlet()`, `setOutletState()` |

### 建议改法

将 `updateShiftRegisters()` 提取为独立的 `ShiftRegisterDriver` 类（约 70 行）：

```
ShiftRegisterDriver
  - void write(ledByte, chip1Byte, chip2Byte)
  - 内部维护 lastShiftData（脏检测）
  - 纯硬件操作，无业务逻辑
```

`Sorter` 持有 `ShiftRegisterDriver` 成员，调用 `driver.write(...)` 替换内联逻辑。

**好处**：`updateShiftRegisters()` 的位图构建逻辑可以独立测试，LED/H桥映射变更不影响 Sorter 主逻辑。  
**风险：低-中。提取纯函数，接口不变。**

---

## R3. ISR 路径中的 Serial 打印 ⭐⭐⭐

### 现状问题（严重性最高）

`sorter.cpp` 的 `run()` 和 `updateShiftRegisters()` 在 1ms 节拍的 `vControlTask` 中循环调用，内部包含多处 `Serial.println/printf()`。115200bps 下发送 30 字节约耗时 ~2.6ms，直接导致控制节拍抖动。

### 待删除的行（共 9 行）

| 行号 | 所在函数 |
|---|---|
| 158 | `run()` — START SCAN 事件打印 |
| 167-169 | `run()` — LATCH DATA 整个 `if` 块含 `Serial.printf` |
| 185 | `run()` — EXECUTE OUTLETS 事件打印 |
| 199 | `run()` — RESET OUTLETS 事件打印 |
| 398, 400, 401, 403, 405 | `updateShiftRegisters()` — 595 数据变化打印 |

### 保留的行（非实时路径）

`initialize()`、`restoreOutletConfig()`、`saveConfig()` 中的打印是一次性启动日志，**保留**。

**风险：低。不影响任何逻辑。**

---

## R4. `onPhaseChange()` 中的 ISR 安全隐患 ⭐⭐

### 现状问题

`onPhaseChange()` 由编码器中断调用（ISR 上下文），但内部调用了 `scanner->stop()`——如果 `stop()` 内有非原子操作则存在风险。

同时，`flagScanStart` / `flagDataLatch` 等标志位使用 `volatile bool`，在多核 ESP32 上仅 `volatile` 不足以保证跨核可见性，应使用 `std::atomic<bool>`。

### 建议改法

1. 将所有标志位从 `volatile bool` 改为 `std::atomic<bool>`（ESP32 Arduino 框架支持）
2. 确认 `DiameterScanner::stop()` 内部不调用任何 FreeRTOS API

**风险：中。需确认 DiameterScanner 实现，改动有限但非零风险。**

---

## R5. `ConfigHandler` 基类死代码 ⭐

### 现状问题

基类的 `checkExit()`、`switchToNextSubMode()`、`handleEncoderInputs()` 等方法永远不会被调用——所有子类均完全覆盖了 `update()`。

### 建议改法

简化基类，只保留公共数据成员和工具方法（如 `handleReturnToMenu()`），将 `update()` 改为纯虚函数，用编译器强制检查取代隐性约定：

```cpp
virtual void update(uint32_t currentMs, bool btnPressed) = 0;
```

**风险：低。只删除死代码，编译器会检查所有子类。**

---

## 优先级汇总

| 项目 | 优先级 | 风险 | 预估改动量 |
|---|---|---|---|
| R3 Serial 打印 | 🔴 最高 | 低 | 删除 9 行 |
| R1 EEPROM 魔法地址 | 🟠 高 | 低 | 改 20 行 |
| R4 ISR 安全隐患 | 🟠 高 | 中 | 改 10 行 + 验证 |
| R2 Sorter 类拆分 | 🟡 中 | 低-中 | 约 80 行新文件 |
| R5 基类清理 | 🟢 低 | 低 | 删除约 30 行 |

> **建议顺序**：R3 → R1 → R4 → R2 → R5
