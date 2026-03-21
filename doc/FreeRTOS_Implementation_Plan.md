# ESP32 分选系统 FreeRTOS 多任务架构方案

## 1. 核心设计思路 (Core Concepts)

为了解决单线程循环导致的 UI 卡顿和控制逻辑时序不稳，系统将切换到基于 FreeRTOS 的抢占式多任务架构。
*   **双核利用**：Core 1 处理实时控制 (Real-time Control)，Core 0 处理 HMI 与后台任务。
*   **任务解耦**：通过队列 (Queue) 和信号量 (Semaphore) 在任务间传递数据，消除 `loop()` 中的阻塞。

---

## 2. 任务清单 (Task Management)

| 任务名称 (Task Name) | 绑定核心 | 优先级 | 频率 (Hz) | 主要职责 (Responsibilities) |
| :--- | :---: | :---: | :---: | :--- |
| **`Control_Task`** | Core 1 | 24 (最高) | 1000 | 编码器跟踪、传送带平移同步、物理出口动作触发。 |
| **`Modbus_Task`** | Core 1 | 20 (高) | 异步 | 管理伺服驱动器状态、读写指令，利用阻塞 IO 减少 CPU 浪费。 |
| **`Sensors_Task`** | Core 1 | 15 (中高) | 200 | 采集直径、电位器及各个接近开关的物理信号。 |
| **`UI_Task`** | Core 0 | 10 (中) | 30 | 处理菜单逻辑、OLED 像素渲染及旋钮输入。 |
| **`System_Task`** | Core 0 | 5 (低) | 5 | EEPROM 数据归档、监控心跳、日志汇总打印。 |

---

## 3. 详细职责描述 (Detailed Responsibilities)

### 3.1 `Control_Task` (实时控制，系统的“大脑”)
*   使用 `vTaskDelayUntil` 保证极高的确定性。
*   根据编码器脉冲数，线性插值计算每一个“在槽位中”物体的当前物理坐标。
*   当物体到达对应的出口脉冲计数值时，立即驱动电磁阀。

### 3.2 `Modbus_Task` (伺服通讯，系统的“骨骼”)
*   重构 `ModbusController`：发送读请求后，任务调用 `xSemaphoreTake` 挂起。
*   串口接收完成中断触发信号量，使任务恢复运行并提取数据。
*   **优点**：不再阻塞循环，极大提高了系统的并行度。

### 3.3 `UI_Task` (人机界面，系统的“外表”)
*   OLED 的渲染耗时集中在此核心。
*   由于 I2C 操作较为耗时且不具实时性，将其完全隔离在 Core 0，避免其抖动影响到 Core 1 的步进脉冲/传感器采集。

### 3.4 `Sensors_Task` (传感器采集，系统的“感觉”)
*   对直径传感器的原始信号进行数字滤波 (Kalman 或 EMA)。
*   当一个完整的物体扫过传感器后，将最终有效直径打包成结构体，推入 `Control_Task` 的输入队列。

---

## 4. 任务间通信与资源竞争 (IPC)

*   **分选队列 (Sorting Queue)**：`Sensors_Task` → `Control_Task`。包含物体直径和其进入传送带的时刻（编码器计数）。
*   **读写互斥锁 (Bus Mutex)**：保护 `ModbusController`。虽然目前只有伺服，但为未来增加更多 RS485 设备做准备。
*   **状态事件组 (Event Group)**：维护全局状态，如 `BIT_SERVO_ALARM`、`BIT_SYSTEM_PAUSED`、`BIT_EEPROM_DIRTY`。

---

## 5. 实施路线图 (Implementation Roadmap)

1.  **Phase 1**：搭建多任务框架，在 `main.cpp` 中创建 `Control_Task` 和 `UI_Task`，将原本的 `loop()` 拆分。
2.  **Phase 2**：实现阻塞式 Modbus 驱动，优化 CPU 资源分配。
3.  **Phase 3**：引入队列机制，使传感器检测与分选动作完全异步化。

---

## 6. 预期效益 (Expected Benefits)

*   **零抖动控制**：分选脉冲响应精度从 5ms 提升至 <1ms。
*   **流畅的 UI**：即使在高速分选时，菜单切换也将保持丝滑。
*   **易于扩展**：增加远程控制（WiFi/MQTT）只需再开一个 Core 0 上的低优先级任务，无需担心影响硬件时序。
