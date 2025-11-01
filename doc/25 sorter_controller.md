# SorterController类实现要求文档

## 1. 概述

SorterController类是ESP32分拣系统的核心控制类，负责协调各个功能模块（包括分支器控制器、托架管理器、编码器等）协同工作，实现完整的物体分拣流程。该类作为系统的中央控制器，管理数据流转、位置跟踪和动作触发，是整个分拣系统的中枢神经系统。

## 2. 核心功能

SorterController提供以下核心功能：

- **系统初始化与配置**：初始化所有子系统，配置分支点位置
- **数据接收与处理**：接收直径扫描仪数据并添加到托架构象
- **编码器脉冲处理**：处理编码器输入，更新系统位置信息
- **分支器控制协调**：基于位置和数据触发分支器动作和复位
- **系统状态管理**：提供启动、停止、重置等系统控制功能
- **测试与诊断**：提供自检和分支器测试功能

## 3. 类定义与成员

### 3.1 公共接口

#### 初始化与配置方法
- `SorterController()`：构造函数，初始化内部状态
- `void initialize(const int servoPins[NUM_DIVERTERS])`：初始化整个分拣系统，包括分支器控制器和托架管理器
- `void setDivergencePoints(const uint8_t positions[NUM_DIVERTERS])`：设置各分支器的分支点位置

#### 数据处理方法
- `void receiveDiameterData(float diameter)`：接收并处理来自直径扫描仪的直径数据
- `void handleEncoderPulses(int pulseCount)`：处理编码器脉冲，更新位置信息
- `void update()`：更新系统状态，检查是否需要触发分支器动作或复位

#### 系统控制方法
- `void start()`：启动分拣系统
- `void stop()`：停止分拣系统并复位分支器
- `void reset()`：完全重置系统状态

#### 状态查询方法
- `uint8_t getCurrentPosition() const`：获取当前传输线位置

#### 测试与调试方法
- `void runSelfTest()`：运行系统自检程序
- `void testDiverter(uint8_t diverterIndex)`：测试指定的分支器
- `void displayCarriageQueue()`：显示当前托架队列状态（调试用）
- `void moveOnePosition()`：模拟移动传输线一个位置

### 3.2 私有成员变量

- `CarriageManager carriageManager`：托架管理器，负责跟踪和管理托架构象
- `DiverterController diverterController`：分支器控制器，控制舵机动作
- `uint8_t currentPosition`：当前传输线位置索引
- `int encoderCount`：编码器脉冲计数
- `bool isRunning`：系统运行状态标志
- `bool activeDiverters[NUM_DIVERTERS]`：跟踪各分支器的激活状态，用于基于位置的复位

### 3.3 常量定义

- `DEFAULT_DIVERGENCE_POINT_1` 至 `DEFAULT_DIVERGENCE_POINT_5`：默认分支点位置
- `ENCODER_PULSES_PER_STEP`：每移动一个位置所需的编码器脉冲数

## 4. 工作流程

### 4.1 系统初始化流程
1. 创建SorterController实例
2. 调用initialize()方法，提供舵机引脚配置
3. 可选：调用setDivergencePoints()自定义分支点位置
4. 调用start()启动系统

### 4.2 数据处理与动作触发流程
1. 通过handleEncoderPulses()接收编码器脉冲，更新位置
2. 系统位置变化时，自动更新托架构象数据
3. 通过receiveDiameterData()接收直径数据，添加到托架构象
4. update()方法定期检查：
   - 是否需要激活分支器（基于位置和直径数据）
   - 是否需要复位分支器（基于位置和激活状态）
5. 分支器复位机制：当物体通过分支点后，在特定位置自动复位

### 4.3 分支器控制逻辑
SorterController实现了精确的分支器控制机制：

1. **激活触发**：当位置到达分支点且有匹配的直径数据时，激活相应分支器
2. **状态跟踪**：使用activeDiverters数组记录各分支器的激活状态
3. **基于位置的复位**：当传输线移动到分支点后的下一个位置时，自动复位该分支器
4. **系统停止处理**：系统停止时，自动复位所有分支器并清除激活状态

## 5. 与其他模块的交互

### 5.1 与DiverterController的交互
- SorterController拥有一个DiverterController实例
- 通过diverterController.activateDiverter()和diverterController.resetDiverter()控制分支器动作
- 系统停止时调用diverterController.resetAllDiverters()复位所有分支器

### 5.2 与CarriageManager的交互
- SorterController拥有一个CarriageManager实例
- 通过carriageManager.addNewDiameterData()添加直径数据
- 通过carriageManager.checkAndExecuteAssignment()检查是否需要激活分支器
- 通过carriageManager.moveCarriages()在位置变化时更新托架构象

### 5.3 与DiagnosticController的交互
- DiagnosticController通过processCurrentMode()方法调用SorterController的功能
- 在不同诊断模式下，DiagnosticController可能调用SorterController的不同方法

## 6. 使用示例

### 6.1 基本使用流程

```cpp
// 创建并初始化分拣控制器
SorterController sorterController;
int servoPins[NUM_DIVERTERS] = {SERVO_PIN_1, SERVO_PIN_2, SERVO_PIN_3, SERVO_PIN_4, SERVO_PIN_5};
sorterController.initialize(servoPins);

// 启动系统
sorterController.start();

// 主循环中
void loop() {
    // 处理编码器脉冲
    sorterController.handleEncoderPulses(newPulses);
    
    // 接收直径数据
    sorterController.receiveDiameterData(measuredDiameter);
    
    // 更新系统状态
    sorterController.update();
}
```

### 6.2 分支点配置

```cpp
// 自定义分支点位置
uint8_t customDivergencePoints[NUM_DIVERTERS] = {3, 7, 12, 18, 24};
sorterController.setDivergencePoints(customDivergencePoints);
```

### 6.3 系统控制

```cpp
// 停止系统
sorterController.stop();

// 重置系统
sorterController.reset();

// 重新启动系统
sorterController.start();
```

### 6.4 测试功能

```cpp
// 运行系统自检
sorterController.runSelfTest();

// 测试特定分支器
sorterController.testDiverter(3); // 测试3号分支器

// 显示当前队列状态（调试用）
sorterController.displayCarriageQueue();
```

## 7. 注意事项

- 初始化时必须提供正确的舵机引脚配置
- 系统运行过程中，应持续调用update()方法以确保状态更新和分支器复位
- 分支器复位基于编码器位置，而非定时器，确保动作精确同步
- 系统停止时会自动复位所有分支器，无需手动操作
- 在诊断模式下，DiagnosticController将接管SorterController的部分功能调用