# UI 输入架构设计文档

**项目**：ESP32 Sorter  
**文件**：`doc/UI_Input_Architecture.md`  
**状态**：设计草案（待实施）  
**日期**：2026-05-04

---

## 1. 问题背景

### 1.1 现状

当前 `UserInterface` 的输入接口直接暴露了编码器的物理语义：

```cpp
// user_interface.h — 现状
int  getEncoderDelta();       // 旋转步数
int  getRawEncoderDelta();    // 原始步数（无分频）
bool isMasterButtonPressed(); // 短按
bool isMasterButtonLongPressed(); // 长按
```

调用链为：
```
菜单逻辑 → UserInterface::getEncoderDelta()
                → RotaryInputSource::getEncoderDelta()
                    → 物理编码器中断计数器
```

### 1.2 问题根因

系统现在接入了三种 UI 设备，它们的**原生交互范式**完全不同：

| 设备 | 原生操作 | 与编码器的兼容性 |
|------|---------|----------------|
| 编码器 + 按钮 | 旋转 N 步 → 按下确认 | ✅ 天然吻合 |
| MCGS 触摸屏 | 直接点击第 N 行 / 直接输入数字 | ❌ 无法用旋转步数表达 |
| Terminal 串口 | 输入命令字符串 / 路径 | ❌ 无法用旋转步数表达 |

> **核心问题**：`getEncoderDelta()` 这类接口把硬件形状（旋转步数）投射到了软件接口上。  
> 让 MCGS 或 Terminal "模拟编码器"是**错误的抽象方向**——用上层语义迁就下层硬件形状。

---

## 2. 设计目标

1. **菜单逻辑与输入设备完全解耦**：菜单不知道输入来自编码器、触摸屏还是串口
2. **每种设备发挥自身优势**：触摸屏可以直接跳转、直接赋值，不强迫它"假装是编码器"
3. **对称于现有 Display 架构**：输入侧与输出侧采用相同的"抽象基类 + 注入"模式
4. **对嵌入式友好**：避免动态内存分配，不引入重型框架

---

## 3. 核心数据结构：UIIntent

### 3.1 设计思路

`UP/DOWN/DELTA/CONFIRM/BACK` 这类事件名虽然比"编码器步数"更抽象，但仍隐含了一个假设：**用户只能通过"小步逼近"来操作**。这是编码器物理限制的软件投影。

真正的解耦应该表达**用户意图（Intent）**，而不是**设备动作（Action on device）**。

### 3.2 定义

```cpp
// 文件建议：src/user_interface/ui_intent.h
// 无任何硬件依赖，可在任何模块引用

enum class UIAction : uint8_t {
    NONE,

    // 导航类
    NAVIGATE_RELATIVE,  // 相对移动 N 步 (param = ±N)
                        // 来源: 编码器旋转 / Terminal 方向键
    NAVIGATE_TO,        // 直接跳到第 N 项 (param = 绝对 index)
                        // 来源: 触摸屏点击行
    NAVIGATE_PATH,      // 按路径寻址 (path = "outlet/0/min")
                        // 来源: Terminal 命令字符串

    // 操作类
    ACTIVATE,           // 确认/展开当前项
                        // 来源: 编码器短按 / 触摸屏点击已选中项 / Enter
    GO_BACK,            // 返回上级
                        // 来源: 编码器长按 / MCGS BACK / Terminal ESC/'q'

    // 值编辑类（编码器范式无法原生表达）
    SET_VALUE,          // 直接写入数值 (param = 新值)
                        // 来源: MCGS 键盘输入 / Terminal "= 20" 命令
};

struct UIIntent {
    UIAction action = UIAction::NONE;
    int      param  = 0;    // NAVIGATE_RELATIVE: 步数(±N)
                            // NAVIGATE_TO: 目标 index
                            // SET_VALUE: 新值（使用项目约定的单位，如 ×100）
    String   path   = "";   // NAVIGATE_PATH 使用，如 "outlet/0/min"
};
```

### 3.3 各设备的意图映射

| 设备原生动作 | UIIntent | 菜单逻辑行为 |
|------------|---------|------------|
| 编码器顺转 +3 步 | `NAVIGATE_RELATIVE {+3}` | `cursor += 3` |
| 编码器短按 | `ACTIVATE {}` | 进入子菜单或执行动作 |
| 编码器长按 | `GO_BACK {}` | 返回上级 |
| MCGS 点击第 3 行 | `NAVIGATE_TO {3}` | `cursor = 3`（O(1) 直达） |
| MCGS 键盘输入 "20" | `SET_VALUE {2000}` | 直接写入，跳过步进调节 |
| MCGS BACK 按钮 | `GO_BACK {}` | 返回上级 |
| Terminal 输入 "w" | `NAVIGATE_RELATIVE {+1}` | `cursor += 1` |
| Terminal 输入 "outlet/0/min" | `NAVIGATE_PATH {"outlet/0/min"}` | 命令行直达 |
| Terminal 输入 "= 20" | `SET_VALUE {2000}` | 直接写入 |

> **关键对比**：  
> - 编码器范式调节一个参数：旋转找到菜单项 → 按下进入 → 旋转逐格调值 → 按下确认（≥4 步）  
> - `SET_VALUE` 范式：MCGS 弹出键盘直接输入（1 步）

---

## 4. 抽象接口：InputSource

### 4.1 接口定义

```cpp
// 文件建议：src/user_interface/input_source.h

#include "ui_intent.h"

class InputSource {
public:
    virtual ~InputSource() = default;

    /**
     * @brief 轮询驱动，在 UI task 的每个 tick 中调用
     *
     * 中断驱动的设备（如编码器）可以空实现此方法。
     * 轮询驱动的设备（如 MCGS Modbus、Terminal Serial）在此方法中
     * 读取原始数据并将其转换为 UIIntent 写入内部队列。
     */
    virtual void tick() = 0;

    /**
     * @brief 查询是否有待处理的意图
     * @return true 表示队列非空，可以调用 pollIntent()
     */
    virtual bool hasIntent() const = 0;

    /**
     * @brief 取出队列中最早的一个意图（FIFO），取后从队列移除
     * @return 下一个 UIIntent，若队列为空则返回 UIAction::NONE
     */
    virtual UIIntent pollIntent() = 0;

    /**
     * @brief 返回设备名称，用于日志和调试
     */
    virtual const char* sourceName() const = 0;
};
```

### 4.2 三种具体实现概要

```
RotaryInputSource
  └── tick(): 空实现（依赖 RotaryInputSource 中断）
  └── hasIntent(): 检查 RotaryInputSource 是否有编码器增量或按钮事件
  └── pollIntent(): 
        编码器增量 ≠ 0  → NAVIGATE_RELATIVE {delta}
        短按            → ACTIVATE
        长按            → GO_BACK

McgsInputSource
  └── tick(): 读取 _cmdRegs（Modbus 轮询结果）
  └── pollIntent():
        HmiCmd::UP/DOWN   → NAVIGATE_RELATIVE {±1}
        触摸行 index      → NAVIGATE_TO {index}
        键盘输入值        → SET_VALUE {value}
        HmiCmd::OK        → ACTIVATE
        HmiCmd::BACK/MENU → GO_BACK / ACTIVATE

TerminalInputSource
  └── tick(): 读取 Serial 缓冲区，逐字符/逐行解析
  └── pollIntent():
        'w'/'s'          → NAVIGATE_RELATIVE {±1}
        数字 + Enter      → NAVIGATE_TO 或 SET_VALUE（按上下文）
        路径字符串        → NAVIGATE_PATH {path}
        Enter            → ACTIVATE
        'q' / ESC        → GO_BACK
```

---

## 5. 集成到 UserInterface

### 5.1 与 Display 对称的注册机制

```cpp
// user_interface.h 扩展

class UserInterface {
    // ── 显示侧（现有）──────────────────────────────
    static const int MAX_DISPLAY_DEVICES = 4;
    Display* displayDevices[MAX_DISPLAY_DEVICES];
    int      displayDeviceCount;

    // ── 输入侧（新增，完全对称）────────────────────
    static const int MAX_INPUT_SOURCES = 4;
    InputSource* inputSources[MAX_INPUT_SOURCES];
    int          inputSourceCount;

public:
    bool addInputSource(InputSource* src);      // 对称于 addDisplayDevice()
    bool removeInputSource(InputSource* src);   // 对称于 removeDisplayDevice()

    // 菜单逻辑调用的唯一输入入口（取代 getEncoderDelta 等）
    UIIntent getNextIntent();
};
```

### 5.2 getNextIntent() 实现

```cpp
UIIntent UserInterface::getNextIntent() {
    for (int i = 0; i < inputSourceCount; i++) {
        inputSources[i]->tick();           // 驱动轮询型设备更新
        if (inputSources[i]->hasIntent()) {
            return inputSources[i]->pollIntent();  // 先注册先服务
        }
    }
    return UIIntent{};  // UIAction::NONE，无输入
}
```

### 5.3 main.cpp 注册示例

```cpp
// setup() 中
// 显示侧（现有模式不变）
ui->addDisplayDevice(&oledDisplay);
ui->addDisplayDevice(&mcgsDisplay);

// 输入侧（新增，对称注册）
ui->addInputSource(&rotaryInput);     // 旋钮编码器（首选，优先级最高）
ui->addInputSource(&mcgsInput);       // MCGS 触摸屏
ui->addInputSource(&terminalInput);   // 串口终端（开发调试用）
```

### 5.4 菜单逻辑调用示例

```cpp
// 菜单 task（原来调用 getEncoderDelta / isMasterButtonPressed）
UIIntent intent = ui->getNextIntent();

switch (intent.action) {
    case UIAction::NAVIGATE_RELATIVE:
        menu->navigate(intent.param);
        break;
    case UIAction::NAVIGATE_TO:
        menu->navigateTo(intent.param);
        break;
    case UIAction::NAVIGATE_PATH:
        menu->navigatePath(intent.path);
        break;
    case UIAction::ACTIVATE:
        menu->activate();
        break;
    case UIAction::GO_BACK:
        menu->goBack();
        break;
    case UIAction::SET_VALUE:
        menu->setValue(intent.param);
        break;
    case UIAction::NONE:
    default:
        break;
}
```

菜单逻辑中**不出现任何设备名称**：无编码器、无 MCGS、无 Terminal。

---

## 6. 可维护性与可读性评估

### 6.1 可维护性

| 维度 | 评估 | 说明 |
|------|------|------|
| 添加新输入设备 | ✅ 极低成本 | 只需实现 `InputSource` 接口（4 个方法），在 setup() 注册 |
| 修改菜单逻辑 | ✅ 无需关心设备 | 菜单只依赖 `UIIntent`，设备增减不影响菜单代码 |
| 修改某个设备驱动 | ✅ 完全隔离 | 只影响对应的 `InputSource` 子类，其他设备不受影响 |
| 单元测试菜单 | ✅ 可注入 Mock | 测试时注入 `MockInputSource`，精确控制输入序列 |
| 多设备并发输入 | ⚠️ 需仲裁策略 | 当前实现为"先注册先服务"，如需要可升级为优先级队列 |

### 6.2 可读性

| 维度 | 评估 | 说明 |
|------|------|------|
| 新人理解代价 | ⭐⭐⭐（中等） | 需理解"意图驱动"模式；熟悉 Command Pattern 的人可直接上手 |
| 调试体验 | ✅ 优秀 | 在 `getNextIntent()` 加一行 log 即可观察所有输入意图，无需分别调试各设备 |
| 接口自解释性 | ✅ 强 | `UIAction::SET_VALUE` 比 `getEncoderDelta()` 含义清晰得多 |
| 与现有代码的一致性 | ✅ 高 | 与 `Display / addDisplayDevice` 模式完全对称，学习曲线平缓 |

### 6.3 复杂度来源与应对

```
复杂度来源 1：UIIntent 类型过多，菜单 switch 分支多
应对：菜单不一定要处理所有 UIAction，
     只处理自己关心的，default: break 忽略其余。

复杂度来源 2：多个 InputSource 同时产生意图时的竞争
应对：当前"轮询顺序即优先级"足够简单。
     未来需要时升级为带优先级的小顶堆，不影响接口。

复杂度来源 3：InputSource 的内部队列管理
应对：每个 InputSource 维护一个深度为 4~8 的静态 ring buffer，
     嵌入式友好，无动态内存分配。
```

---

## 7. 与现有代码的兼容路径

本架构**不要求一次性全量替换**。可以分阶段实施：

### 阶段 0（当前）：不修改任何代码
仅记录设计，用于团队对齐和后续评审。

### 阶段 1：添加新文件，不破坏旧接口
- 新增 `ui_intent.h`（纯数据结构，无依赖）
- 新增 `input_source.h`（纯抽象接口）
- `UserInterface` 新增 `addInputSource()` 和 `getNextIntent()`
- 旧的 `getEncoderDelta()` 等接口**保留不动**

### 阶段 2：实现 RotaryInputSource
- 包装现有 `RotaryInputSource`，内部调用 `getEncoderDelta()` 等
- 菜单逻辑切换为调用 `getNextIntent()`（替换旧调用）
- 旧接口变为内部实现细节，可以 deprecated 标注

### 阶段 3：实现 McgsInputSource
- 从 `McgsDisplay::handleCmdCode()` 中提取命令解析逻辑
- 将注释中预留的 `injectKeyEvent()` 替换为 `pushIntent()`

### 阶段 4：实现 TerminalInputSource（可选）
- 开发阶段使用串口调试菜单，无需连接实体硬件

---

## 8. 文件结构规划

```
src/user_interface/
├── ui_intent.h           ← 新增：UIAction + UIIntent（无硬件依赖）
├── input_source.h        ← 新增：InputSource 纯虚接口
├── rotary_input.h/.cpp   ← 新增：RotaryInputSource（已重命名）
├── mcgs_input.h/.cpp     ← 新增：McgsInputSource（从 McgsDisplay 提取）
├── terminal_input.h/.cpp ← 可选：TerminalInputSource
├── user_interface.h/.cpp ← 修改：addInputSource / getNextIntent
├── RotaryInputSource.h/.cpp  ← 已重命名：原 SimpleHMI，现在直接作为 RotaryInputSource 实现
├── mcgs_display.h/.cpp   ← 精简：handleCmdCode 逻辑移入 McgsInputSource
├── display.h             ← 不变：Display 抽象基类（输出侧对称参考）
└── ...
```

---

## 9. 设计原则总结

> **这个架构的核心哲学**：  
> `InputSource` 不是"编码器的抽象"，而是"**用户意图来源**"的抽象。  
> 任何能产生 `UIIntent` 的事物都可以成为 `InputSource`——  
> 无论是中断驱动的旋钮、Modbus 轮询的触摸屏、串口命令，还是未来的蓝牙遥控。

```
物理世界                    抽象边界                   应用逻辑
────────                    ────────                   ────────
编码器中断   ────────────► │              │ ─────────► UIIntent
Modbus 寄存器 ───────────► │ InputSource  │ ─────────► 消费者只
Serial 字符   ───────────► │  (纯虚接口) │ ─────────► 认识 UIAction
未来蓝牙     ────────────► │              │ ─────────► 和 param/path
```
