# R3 — 删除实时路径中的 Serial 打印

## 背景

`sorter.cpp` 的 `run()` 和 `updateShiftRegisters()` 在 `vControlTask`（1ms 节拍，Core 1）中循环调用。
`Serial.println()` 在 115200bps 下发送约 30 字节需 ~2.6ms，直接导致控制节拍抖动。

## 删除范围

### 删除（实时路径，必须清除）

| 行号 | 内容 | 所在函数 |
|---|---|---|
| 158 | `Serial.println("[SORTER] Event -> START SCAN (50)")` | `run()` |
| 167-169 | `Serial.printf("[SORTER] Event -> LATCH DATA...")` | `run()` |
| 185 | `Serial.println("[SORTER] Event -> EXECUTE OUTLETS (30)")` | `run()` |
| 199 | `Serial.println("[SORTER] Event -> RESET OUTLETS (150)")` | `run()` |
| 398 | `Serial.printf("[595] Data Change! -> 24-bit: ...")` | `updateShiftRegisters()` |
| 400 | `Serial.printf("O%d(Open) ", i)` | `updateShiftRegisters()` |
| 401 | `Serial.printf("O%d(Close) ", i)` | `updateShiftRegisters()` |
| 403 | `Serial.println()` | `updateShiftRegisters()` |
| 405 | `Serial.printf("[595] Data Change! ...")` | `updateShiftRegisters()` |

### 保留（非实时路径，一次性调用，无影响）

| 行号 | 内容 | 所在函数 |
|---|---|---|
| 50 | `"Sorter components initialized..."` | `initialize()` |
| 67, 80 | `"[Sorter] Restoring / EEPROM empty..."` | `restoreOutletConfig()` |
| 192 | （已注释掉，不处理） | `run()` |
| 423, 436 | `"[Sorter] Saving configuration..."` | `saveConfig()` |

## 操作步骤

1. 打开 `src/modular/sorter.cpp`
2. 删除上表"删除"栏的所有行（9 行）
3. 对于 167-169 的 `if (diameterMm > 0) { ... }` 块，整个 `if` 块一并删除

## 预期效果

- `run()` 平均执行时间从 ~3-5ms（有事件时）降至 <0.1ms
- 1ms 控制节拍恢复确定性
- 串口不再有运行期刷屏，调试输出保持清晰
