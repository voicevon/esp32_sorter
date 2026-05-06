# ESP32 Sorter 显示层重构计划

## 1. 架构核心目标
基于“数据驱动”与“按需拉取”思想，彻底解决多任务环境下的 UI 撕裂问题与 CPU 算力浪费。
*   **极致解耦**：显示驱动 (Consumer) 不再依赖具体业务模块 (Producer)，它们之间唯一的桥梁是纯数据结构体 `DisplaySnapshot`。
*   **内存复用**：利用 `union` 管理互斥的 APP 页面数据，保证结构体处于极小体积。
*   **硬实时与线程安全**：生产端与消费端均以微秒级耗时持有互斥锁 (`Mutex`) 进行数据 Copy，确保不阻塞核心中断。
*   **按需同步**：采用 10Hz 定时器触发中间代理 (Broker) 拉取数据，不盲目推送。

---

## 2. 第一阶段：定义统一数据标准 (Data Standard)
**目标文件**：新建 `src/user_interface/common/display_types.h`

**计划行动**：
1.  定义各 APP 专属的子快照结构体，例如：
    *   `DashboardSnapshot` (速度、产量、直径)
    *   `EncoderDiagSnapshot` (原生值、修正值、逻辑值、零位偏移)
    *   `LaserDiagSnapshot` (波形长度、实时状态掩码、固定大小的历史波形数组)
    *   `OutletConfigSnapshot` (出口参数)
2.  定义顶层包装类 `DisplaySnapshot`：
    *   包含全局标志：`SystemMode currentMode` 或 `String activePage`。
    *   包含核心数据区：一个包罗万象的 `union`，内部包含上述所有子快照结构体。

---

## 3. 第二阶段：接口层“倒置” (Interfaces Refactoring)
**目标文件**：`src/apps/IApp.h` (或相应基类) & `src/user_interface/common/display.h`

**计划行动**：
1.  **IApp 层升级**：
    *   新增纯虚函数 `virtual void captureSnapshot(DisplaySnapshot& snapshot) = 0;`。
    *   该接口明确要求：由各子 APP 自己决定如何把数据塞进 `DisplaySnapshot` 对应的 `union` 分支中。
2.  **Display 基类精简**：
    *   删除所有繁杂的特定业务渲染函数（如 `displayOutletStatus`、`displayScannerEncoderValues`）。
    *   仅保留核心函数：`virtual void refresh(const DisplaySnapshot& snapshot) = 0;`。

---

## 4. 第三阶段：生产端改造 (Producers: Handlers/Apps)
**目标文件**：`SystemProg` 或各 `DiagnosticHandler`

**计划行动**：
以 `DiagnosticHandler` 为例，实现 `captureSnapshot` 方法：
1.  加锁：请求自身数据模型的 `Mutex`。
2.  组装：判断自身的激活状态（如属于编码器还是激光），然后将内部 `volatile` 数据迅速拷贝进传入的 `snapshot.union_data.laser` 等对应内存区。
3.  解锁：释放 `Mutex`（全程仅内存操作，耗时微秒级）。

---

## 5. 第四阶段：消费端改造 (Consumers: Displays)
**目标文件**：`OledDisplay` 和 `Rs485TouchScreen`

**计划行动**：
1.  **Rs485TouchScreen**：
    *   重写 `refresh()`：根据传入快照的 `activePage`，将其 `union` 中对应的数据直接序列化为 JSON。
    *   移除内部缓存逻辑，真正实现“发完即忘”。
2.  **OLED Display**：
    *   重写 `refresh()`：根据 `currentMode` 或 `activePage`，调度内部的 U8g2 画图逻辑。

---

## 6. 第五阶段：闭环代理与调度 (Broker / Scheduler)
**目标文件**：`SystemManager` 或核心 UI 任务 (如 `tick()`)

**计划行动**：
1.  设置一个固定的 UI 刷新定时器（例如 100ms / 10Hz）。
2.  在定时器回调中执行以下固定流程：
    *   获知当前处于活跃状态的 `IApp`。
    *   在栈上实例化一个纯净的 `DisplaySnapshot`。
    *   执行 `activeApp->captureSnapshot(snapshot);` （从生产者拉取最新快照）。
    *   遍历已注册的所有显示设备（OLED、RS485），分别执行 `display->refresh(snapshot);`（推给消费者渲染）。

---

## 7. 风险评估与验证要求
*   **联合体 (Union) 尺寸对齐**：需确保各子快照内的数组大小合理，防止联合体体积超过单片机合理栈大小。
*   **Mutex 死锁防护**：在重构生产端的 `captureSnapshot` 时，严禁在持锁期间调用任何可能阻塞或延时的操作（如串口打印、延时等）。
