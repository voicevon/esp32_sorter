# DiameterScanner类实现要求文档

## 1. 概述

DiameterScanner类用于封装ESP32分拣系统中的直径扫描仪功能，提供物体检测、数据采样和直径计算能力。

## 2. 直径扫描仪技术规格

- **类型**：单点直径扫描仪
- **输出信号**：数字信号（TTL电平）
- **信号判断**：
  - 有物体挡住：输出低电平
  - 无物体挡住：输出高电平
- **距离判断**：
  - 距离小于阈值：输出低电平
  - 距离大于阈值：输出高电平

## 3. 类定义与成员

### 3.1 公共接口

- **初始化方法**
  - `void initialize()`：初始化扫描仪引脚和内部缓冲区
  - `void reset()`：重置扫描仪状态和缓冲区

- **数据采集方法**
  - `void sample(long encoderPosition)`：在指定编码器位置采样扫描仪状态，将数据存入缓冲区，并更新nextSampleIndex
  - `bool isObjectDetected()`：实时检测当前是否有物体（通过读取扫描仪引脚状态）
  
- **状态管理方法**
  - `void processData()`：处理采样缓冲区中的数据，计算物体直径，并将结果存储到detectedDiameter变量中
  - `float getDetectedDiameter()`：获取最近计算的物体直径值（前提是isDataReady()返回true）
  - `bool isDataReady()`：检查是否有新的直径数据可供读取（processData()已完成计算）

- **配置方法**
  - `void setSamplingThreshold(int threshold)`：设置采样阈值

- **调试信息方法**
  - `void printStatus()`：输出扫描仪当前状态信息
  - `void enableDebug(bool enable)`：启用或禁用调试模式

### 3.2 静态成员

- **常量定义**
  - `static const int SAMPLE_BUFFER_SIZE`：采样缓冲区大小
  - `static const int DATA_PROCESSING_PHASE`：数据处理相位（85相位）

### 3.3 私有成员变量

- **扫描仪配置**
  - `int scannerPin`：扫描仪信号引脚
  - `int samplingThreshold`：采样阈值

- **数据缓冲区**
  - `bool sampleBuffer[SAMPLE_BUFFER_SIZE]`：存储采样数据的缓冲区
  - `int nextSampleIndex`：下一个要采样的索引位置（初始化为0）

- **处理结果**
  - `float detectedDiameter`：通过processData()计算得出的物体直径值
  - `bool dataReady`：直径计算完成且可通过getDetectedDiameter()读取的标志

- **调试标志**
  - `bool debugEnabled`：调试模式标志

## 4. 采样处理机制

- **采样流程**：系统在每个编码器相位调用sample()方法记录扫描仪状态
- **数据存储**：sample()方法将采样状态存入循环缓冲区，nextSampleIndex递增并循环
- **数据处理**：系统在编码器第85相位位置调用processData()方法
- **直径计算**：processData()方法通过统计连续低电平时间确定物体直径
  - 当物体通过激光传感器时，会遮挡激光，导致传感器输出低电平
  - 系统通过测量连续低电平的采样点数来判断物体的直径
  - 具体处理过程：
    1. 编码器触发采样事件
    2. 读取激光传感器的当前状态（高电平或低电平）
    3. 连续记录低电平的采样点数
    4. 当检测到电平变化（从低到高）时，计算连续低电平的总点数
    5. 根据连续低电平的点数，转换为对应的物体直径值
    6. 将计算出的直径值存储并进行后续处理
- **结果输出**：processData()方法将计算结果存储在detectedDiameter变量中，并设置dataReady标志
- **数据读取**：系统通过isDataReady()检查数据是否可用，然后通过getDetectedDiameter()获取计算结果

## 5. 与编码器的协作

- 接收编码器提供的位置信息进行同步采样
- 在特定位置（第85相位）执行数据处理
> **重要说明**：相位范围是0-199，代表物理上一圈的位置。数据处理在第85相位位置执行。关于编码器与传送带、出口的具体对应关系，请参考[传送带系统文档](convenyor.md)。
- 为分拣系统提供物体直径数据，用于在对应托架构象位置控制分支器决策