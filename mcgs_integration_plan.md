# 昆仑通态 (MCGS) 串口屏深度集成实施计划 (V4)

本方案不仅涵盖 ESP32 的代码集成，还包含了对 MCGS 组态逻辑的"协议主导"设计，以及各画面的详细布局与交互逻辑。

---

## 1. 寄存器地址分配 (主导协议)

采用 **Modbus 4xxxx (保持寄存器)** 区进行通讯。
*注：MCGS 中地址从 1 开始，对应 Modbus 通讯中的 0 地址。*

### A. 系统状态区 (Screen Read Only)

| Modbus 地址 | 变量名          | 功能说明           | 数据格式 | 备注                                  |
| :---------- | :-------------- | :----------------- | :------- | :------------------------------------ |
| `40001`     | `SysMode`       | 系统运行模式       | uint16   | 0:Normal, 1:DiagEncoder, 2:DiagScanner, 3:DiagOutlet |
| `40002`     | `LedBitmap`     | 指示LED状态位图    | uint16   | bit0=出口0, bit1=出口1 … bit7=出口7   |
| `40003`     | `ConveyorLoad`  | 传送带运行负载率   | uint16   | 0–1000 (代表 0.0–100.0%)              |
| `40004`     | `FirmwareVer`   | 固件版本 (BCD码)   | uint16   | 例：0x0200 = v2.0.0                   |

### B. 生产数据区 (Screen Read Only)

| Modbus 地址 | 变量名          | 功能说明           | 数据格式 | 备注                                 |
| :---------- | :-------------- | :----------------- | :------- | :----------------------------------- |
| `40010`     | `DiameterNow`   | 实时测量直径       | uint16   | ×100，例 2250 = 22.50 mm             |
| `40011`     | `SpeedPerHour`  | 分拣速度           | uint16   | 单位 pcs/h                           |
| `40012`     | `TotalCnt_Hi`   | 总计数高位         | uint16   | 与 40013 合并为 uint32               |
| `40013`     | `TotalCnt_Lo`   | 总计数低位         | uint16   |                                      |
| `40014`     | `IdentCnt_Hi`   | 识别计数高位       | uint16   | 与 40015 合并为 uint32               |
| `40015`     | `IdentCnt_Lo`   | 识别计数低位       | uint16   |                                      |
| `40016`     | `DiagValue1`    | 诊断辅助值1        | uint16   | 模式相关（编码器相位 / 扫描脉宽 等） |
| `40017`     | `DiagValue2`    | 诊断辅助值2        | uint16   | 模式相关                             |

### C. 配置参数区 (Screen Read Only — 上电同步)

| Modbus 地址 | 变量名              | 功能说明             | 数据格式 | 备注                   |
| :---------- | :------------------ | :------------------- | :------- | :--------------------- |
| `40020`     | `Outlet0_Min`       | 出口0 直径下限       | uint16   | ×100，mm               |
| `40021`     | `Outlet0_Max`       | 出口0 直径上限       | uint16   | ×100，mm               |
| `40022`     | `Outlet0_LenMask`   | 出口0 长度掩码       | uint16   | 位图：bit0=S,1=M,2=L   |
| …           | …                   | …（每出口 3 个寄存器）| …        | 共8个出口，地址到 40043 |
| `40044`     | `Outlet0_Mode`      | 出口0 工作模式       | uint16   | 0=多物体, 1=直径       |

### D. 触控控制区 (Read Write)

| Modbus 地址 | 变量名        | 功能说明             | 数据格式 | 备注                               |
| :---------- | :------------ | :------------------- | :------- | :--------------------------------- |
| `40100`     | `CmdCode`     | 触控命令码（握手）   | uint16   | 1:MENU, 2:OK, 3:UP, 4:DOWN, 5:BACK; **ESP32 执行后立即回写 0** |
| `40101`     | `ParamIdx`    | 参数修改索引         | uint16   | 0–7：目标出口编号                  |
| `40102`     | `ParamField`  | 参数修改字段         | uint16   | 0:Min, 1:Max, 2:LenMask, 3:Mode   |
| `40103`     | `ParamValue`  | 参数修改数值         | uint16   | 屏幕写入的新值（×100 单位）        |
| `40104`     | `CmdConfirm`  | 写入确认命令         | uint16   | 1:WRITE_EEPROM; ESP32 执行后回写 0 |

---

## 2. MCGS 组态画面设计

### 画面总览（Page Map）

```
[P0: 开机启动画面] ──► [P1: 主仪表盘] ──► [P2: 配置总览]
                              │                     │
                              ▼                     ▼
                       [P3: 诊断画面]        [P4: 出口配置详情]
                              │
                        (按模式切换子区域)
```

---

### P0 — 开机启动画面

**目的**：品牌展示 + 通讯建立检测。

| 元素       | 控件类型       | 描述                                               |
| :--------- | :------------- | :------------------------------------------------- |
| Logo       | 位图控件       | 公司/产品 Logo                                     |
| 系统名称   | 标签           | "芦笋分拣系统 V2"                                  |
| 通讯状态   | 指示灯控件     | 读取 `SysMode` ≠ 0xFFFF → 绿灯"已连接"；否则红灯  |
| 固件版本   | 文本控件       | 显示 `FirmwareVer` BCD 解码后的字符串              |
| 自动跳转   | 定时器脚本     | 连接成功后 2s 自动跳转 P1；否则停留并提示检查接线  |

---

### P1 — 主仪表盘画面

**目的**：生产运行时的核心监控视图。

#### 布局草图（800×480 横屏参考）

```
┌─────────────────────────────────────────────────────────┐
│  [状态栏] 系统: ●运行  模式: 正常生产   时间: 14:35:21  │
├──────────────┬──────────────┬───────────────────────────┤
│              │   直径仪表盘  │   产量统计                │
│  出口状态    │  ┌────────┐  │  总计:   000123 pcs       │
│  灯阵        │  │ 22.50mm│  │  识别:   000119 pcs       │
│  ○○○○○○○○   │  │ ████▌  │  │  速度:   3600   pcs/h     │
│  (8个出口)   │  └────────┘  │                           │
│              │              │  传送带负载: [███░░] 72%  │
├──────────────┴──────────────┴───────────────────────────┤
│  [按钮区]  [⚙ 配置]   [🔧 诊断]   [🧹 清零计数]        │
└─────────────────────────────────────────────────────────┘
```

#### 控件详表

| 控件名称         | 控件类型         | 绑定变量             | 显示逻辑 / 脚本                                   |
| :--------------- | :--------------- | :------------------- | :------------------------------------------------ |
| `lbl_mode`       | 标签             | `SysMode`            | 脚本映射：0→"正常生产", 1→"诊断-编码器" 等        |
| `gauge_diameter` | 仪表盘 / 数值    | `DiameterNow`        | 显示值 = `DiameterNow` / 100，格式 "##.## mm"     |
| `bar_diameter`   | 进度条           | `DiameterNow`        | 量程 0–5000 (0–50mm)，反映实时直径                |
| `lbl_total`      | 标签             | `TotalCnt_Hi/Lo`     | 脚本：`val = TotalCnt_Hi * 65536 + TotalCnt_Lo`   |
| `lbl_ident`      | 标签             | `IdentCnt_Hi/Lo`     | 同上                                              |
| `lbl_speed`      | 标签             | `SpeedPerHour`       | 直接绑定，单位 pcs/h                              |
| `bar_conveyor`   | 进度条           | `ConveyorLoad`       | 量程 0–1000                                       |
| `led_outlet[0..7]` | 指示灯 ×8     | `LedBitmap`          | 脚本：`led_outlet[i].Color = (LedBitmap >> i) & 1 ? 绿 : 灰` |
| `btn_config`     | 按钮             | —                    | 点击 → 写 `CmdCode=1` (MENU)，跳转 P2             |
| `btn_diag`       | 按钮             | —                    | 点击 → 跳转 P3                                    |
| `btn_clear`      | 按钮             | —                    | 点击 → 弹出确认对话框；确认后写 `CmdCode=5`       |

---

### P2 — 配置总览画面

**目的**：以表格形式一览所有 8 个出口的分拣参数，并提供入口进入单出口编辑。

#### 布局草图

```
┌──────────────────────────────────────────────────────────┐
│  ← 返回       出口分拣配置                               │
├──────┬────────┬────────┬──────────┬───────────┬─────────┤
│ 出口 │ 最小mm │ 最大mm │ 长度掩码 │  工作模式 │  操作   │
├──────┼────────┼────────┼──────────┼───────────┼─────────┤
│  0   │ 15.00  │ 18.00  │  S+M+L   │  直径模式 │ [编辑]  │
│  1   │ 18.00  │ 21.00  │   M+L    │  直径模式 │ [编辑]  │
│  …   │  …     │  …     │   …      │     …     │  …      │
│  7   │  0.00  │  0.00  │  (废弃)  │  多物体   │ [编辑]  │
└──────┴────────┴────────┴──────────┴───────────┴─────────┘
│  [← 返回]                                [📥 同步EEPROM] │
└──────────────────────────────────────────────────────────┘
```

#### 控件详表

| 控件名称           | 控件类型 | 绑定变量/动作                                        |
| :----------------- | :------- | :--------------------------------------------------- |
| `tbl_outlet_min[i]`| 标签×8   | `Outlet{i}_Min / 100`，格式 "##.##"                  |
| `tbl_outlet_max[i]`| 标签×8   | `Outlet{i}_Max / 100`，格式 "##.##"                  |
| `tbl_len_mask[i]`  | 标签×8   | 脚本映射位图：0x07→"S+M+L", 0x06→"M+L" 等           |
| `tbl_mode[i]`      | 标签×8   | `Outlet{i}_Mode`: 0→"多物体", 1→"直径"               |
| `btn_edit[i]`      | 按钮×8   | 点击 → 写 `ParamIdx=i`，跳转 P4                      |
| `btn_sync_eeprom`  | 按钮     | 点击 → 弹确认框；确认后写 `CmdConfirm=1`              |
| `btn_back`         | 按钮     | 点击 → 写 `CmdCode=5` (BACK)，跳转 P1               |

> **上电同步逻辑（脚本）**：P2 进入时触发一次批量读取 `Outlet0_Min` ~ `Outlet0_Mode`（MCGS 驱动已自动轮询，此处仅刷新标签显示）。

---

### P4 — 出口配置详情画面（单出口编辑）

**目的**：对选定出口的 `Min/Max/LenMask/Mode` 进行精确数值输入与实时预览。

#### 布局草图

```
┌──────────────────────────────────────────────┐
│  ← 返回      编辑出口 #N 配置                 │
├──────────────────────────────────────────────┤
│  最小直径 (mm):   [  18.00  ] ▲ ▼            │
│  最大直径 (mm):   [  21.00  ] ▲ ▼            │
│                                              │
│  长度选择:  [ ■ S ]  [ ■ M ]  [ □ L ]       │
│  (复选框，勾选后实时更新位图预览)             │
│                                              │
│  工作模式:  ● 直径模式   ○ 多物体模式        │
│                                              │
│  ──────── 实时预览 ───────────────           │
│  当前直径: 22.50mm  → ✅ 命中此出口          │
├──────────────────────────────────────────────┤
│  [取消]                    [✔ 写入并保存]    │
└──────────────────────────────────────────────┘
```

#### 控件详表

| 控件名称         | 控件类型         | 绑定变量 / 动作                                             |
| :--------------- | :--------------- | :---------------------------------------------------------- |
| `lbl_outlet_idx` | 标签             | 显示 `ParamIdx` 值（"编辑出口 #N"）                         |
| `num_min`        | 数值输入控件     | 内部变量 `EditMin`；初始值 = `Outlet{N}_Min`；步进 +1 (-1)  |
| `num_max`        | 数值输入控件     | 内部变量 `EditMax`；初始值 = `Outlet{N}_Max`                |
| `chk_len_S`      | 复选框           | 绑定内部 `EditLenMask` bit0                                 |
| `chk_len_M`      | 复选框           | 绑定内部 `EditLenMask` bit1                                 |
| `chk_len_L`      | 复选框           | 绑定内部 `EditLenMask` bit2                                 |
| `radio_mode_dia` | 单选按钮         | `EditMode = 1`                                              |
| `radio_mode_multi`| 单选按钮        | `EditMode = 0`                                              |
| `lbl_preview`    | 标签             | 脚本：比较 `DiameterNow` 与 `EditMin/Max`，显示"命中/未命中"|
| `btn_save`       | 按钮             | 见下方写入时序                                              |
| `btn_cancel`     | 按钮             | 跳转回 P2，不写入                                           |

#### 写入时序（`btn_save` 脚本）

```vbscript
' Step 1: 写入 Min 值
ParamIdx   = N           ' 已由 P2 的 btn_edit 设置
ParamField = 0           ' Min
ParamValue = EditMin     ' 已乘100的整数值
CmdCode    = 2           ' OK (触发 ESP32 暂存)
Wait 200ms
CmdCode    = 0           ' 清零

' Step 2: 写入 Max 值
ParamField = 1
ParamValue = EditMax
CmdCode    = 2
Wait 200ms
CmdCode    = 0

' Step 3: 写入 LenMask
ParamField = 2
ParamValue = EditLenMask
CmdCode    = 2
Wait 200ms
CmdCode    = 0

' Step 4: 写入 Mode
ParamField = 3
ParamValue = EditMode
CmdCode    = 2
Wait 200ms
CmdCode    = 0

' Step 5: 触发 EEPROM 持久化
CmdConfirm = 1
Wait 500ms
CmdConfirm = 0

' Step 6: 跳转回 P2
GotoPage(P2)
```

---

### P3 — 诊断画面

**目的**：实时显示 `DiagValue1/2`，用于现场调试编码器相位或扫描仪脉宽。

| 元素           | 控件类型 | 绑定变量      | 说明                           |
| :------------- | :------- | :------------ | :----------------------------- |
| `lbl_diagmode` | 标签     | `SysMode`     | 当前诊断子模式名称             |
| `val_diag1`    | 数值标签 | `DiagValue1`  | 主诊断值（如编码器相位 0-199） |
| `val_diag2`    | 数值标签 | `DiagValue2`  | 辅诊断值（如激光脉宽）         |
| `trend_diag1`  | 趋势曲线 | `DiagValue1`  | 实时波形，采样间隔 100ms       |
| `btn_back`     | 按钮     | —             | 写 `CmdCode=5`，跳转 P1        |

---

## 3. ESP32 侧架构变更

### [NEW] 模块：`ModbusDisplay` (继承 `Display`)

**文件**：`src/user_interface/modbus_display.h / .cpp`

**职责**：
- 在 `vUITask` 中，每 **50ms** 通过 `ModbusMaster` 异步读取 `CmdCode` (40100)。
- 在 `vControlTask` 推送数据时，通过 `asyncWrite` 更新 40010–40017 区段。
- **防抖握手**：读到 `CmdCode` 非零 → 立即执行对应动作 → 异步回写 `CmdCode = 0`。
- **配置同步**：上电后将 EEPROM 中的直径区间批量写入 40020–40043，供屏幕上电读取。

```cpp
// 核心接口示意 (modbus_display.h)
class ModbusDisplay : public Display {
public:
    void initialize() override;
    void pollCommand();               // 每50ms 在 vUITask 中调用
    void pushProductionData(         // 每500ms 在 vControlTask 中调用
        uint16_t diameter,
        uint16_t speedPerHour,
        uint32_t totalCount,
        uint32_t identCount
    );
    void syncConfigToScreen();        // 上电时将 EEPROM 配置推送到屏幕寄存器
private:
    void handleCmdCode(uint16_t cmd);
    void handleParamWrite(uint16_t idx, uint16_t field, uint16_t value);
    ModbusMaster* _master;
};
```

### [MODIFY] 模块：`UserInterface`

- 增加 `injectKeyEvent(KeyEvent e)` 方法，允许 `ModbusDisplay` 注入与实体旋钮相同的按键事件。
- `CmdCode` 映射：`1(MENU)→KEY_BTN_LONG`, `2(OK)→KEY_BTN_CLICK`, `3(UP)→KEY_ENC_UP`, `4(DOWN)→KEY_ENC_DOWN`, `5(BACK)→KEY_BTN_CLICK（在上层菜单）`。

### [MODIFY] 模块：`config.h`

新增 RS485 引脚定义：
```cpp
// HMI 485 Serial (UART2)
constexpr int PIN_HMI_485_TX = 17;
constexpr int PIN_HMI_485_RX = 16;
constexpr int PIN_HMI_485_EN = 5;   // 驱动方向控制（TX/RX 切换）
constexpr int HMI_MODBUS_SLAVE_ID = 1;
constexpr int HMI_MODBUS_BAUD = 115200;
```

### [NEW] 头文件：`hmi_registers.h`

```cpp
// src/user_interface/hmi_registers.h
// ── 状态区 ──
constexpr uint16_t HMI_REG_SYS_MODE      = 0;   // 40001
constexpr uint16_t HMI_REG_LED_BITMAP    = 1;   // 40002
constexpr uint16_t HMI_REG_CONVEYOR_LOAD = 2;   // 40003
// ── 生产数据区 ──
constexpr uint16_t HMI_REG_DIAMETER      = 9;   // 40010
constexpr uint16_t HMI_REG_SPEED_PH      = 10;  // 40011
constexpr uint16_t HMI_REG_TOTAL_HI      = 11;  // 40012
constexpr uint16_t HMI_REG_TOTAL_LO      = 12;  // 40013
constexpr uint16_t HMI_REG_IDENT_HI      = 13;  // 40014
constexpr uint16_t HMI_REG_IDENT_LO      = 14;  // 40015
// ── 配置区 (每出口 3 个寄存器：Min, Max, LenMask) ──
constexpr uint16_t HMI_REG_CFG_BASE      = 19;  // 40020
// ── 触控控制区 ──
constexpr uint16_t HMI_REG_CMD_CODE      = 99;  // 40100
constexpr uint16_t HMI_REG_PARAM_IDX     = 100; // 40101
constexpr uint16_t HMI_REG_PARAM_FIELD   = 101; // 40102
constexpr uint16_t HMI_REG_PARAM_VALUE   = 102; // 40103
constexpr uint16_t HMI_REG_CMD_CONFIRM   = 103; // 40104
```

---

## 4. 实施路径

### 第一阶段：基础设施

1. 移植 `ModbusMaster.h/cpp` 到 `src/modular/`。
2. 在 `config.h` 中补充 485 引脚常量。
3. 创建 `hmi_registers.h`，硬编码全部地址映射。

### 第二阶段：ESP32 驱动层

1. 实现 `ModbusDisplay` 骨架（`initialize` + `pollCommand` + `pushProductionData`）。
2. 用 **Modbus Poll** 在电脑端模拟 MCGS，验证读写通讯正常。
3. 在 `UserInterface` 增加 `injectKeyEvent`，验证菜单逻辑复用。

### 第三阶段：配置写入链路

1. 实现 `handleParamWrite` 和 `syncConfigToScreen`。
2. 验证：屏幕写入 `ParamValue → CmdCode=2` → ESP32 更新内存 → `CmdConfirm=1` → EEPROM 持久化。

### 第四阶段：MCGS 组态工程

1. 按本文 §2 逐一创建 P0~P4 画面。
2. 在 MCGS 驱动中新建 `Modbus RTU Master`，按地址表批量导入变量。
3. 编写各按钮的动作脚本（参考 P4 写入时序示例）。
4. 用真实屏幕联调，修正通讯超时 / 握手时序。

---

## 5. 验证方案

| 测试项             | 工具                  | 通过条件                                      |
| :----------------- | :-------------------- | :-------------------------------------------- |
| 通讯层验证         | Modbus Poll           | 能正常读写 40001~40104，无 CRC 错误           |
| 仪表盘数据刷新     | 真实屏幕 P1 画面      | 直径、速度、计数与 OLED 显示数据一致          |
| 出口灯阵同步       | 真实屏幕 P1 灯阵      | 手动触发出口时，LED 位图与实体灯同步变化      |
| 参数写入链路       | 真实屏幕 P4 写入流程  | 写入后 MCGS 读回数值与写入值一致；重启后持久  |
| 按键注入           | 屏幕虚拟按钮          | 屏幕 MENU/OK/UP/DOWN 与实体旋钮行为完全一致   |

---

> [!IMPORTANT]
> **部署工具说明**
> MCGS 工程文件（`.MCG`）需在 MCGS 组态软件中手动创建。本文档中的变量名、地址表和脚本逻辑可直接复制粘贴到组态软件对应字段，通过"批量增加变量"功能快速导入。
