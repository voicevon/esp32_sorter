# ESP32 Sorter 项目重构建议

## 二级重构建议（命名优化、代码结构改进）

### 1. 命名优化

#### 1.1 变量命名优化
- **文件**: `src/modular/encoder.h`
  - `encoderCount` → `rawEncoderCount`（更清晰地表示这是原始计数值）
  - `previousCount` → `lastEncoderCount`（与其他类中的命名保持一致，如 `lastUpdateTime`）

- **文件**: `src/modular/diameter_scanner.h`
  - `highLevelPulseCounts` → `pulseCounts`（更简洁，上下文已明确是高电平）
  - `nominalDiameter` → `currentDiameter`（更准确地表示这是当前计算的直径值）

- **文件**: `src/modular/sorter.h`
  - `restartScan` → `shouldRestartScan`（更清晰地表示这是一个标志位）
  - `calculateDiameter` → `shouldCalculateDiameter`（同上）

#### 1.2 方法命名优化
- **文件**: `src/modular/diameter_scanner.h`
  - `getDiameterAndStop()` → `getDiameterAndReset()`（更准确地表示操作，因为它是重置而不是停止）

- **文件**: `src/modular/sorter.h`
  - `encoderPhaseCallback()` → `onEncoderPhaseChange()`（更符合事件处理方法的命名规范）

- **文件**: `src/user_interface/oled.h`
  - `drawHeader()` → `renderHeader()`（与现代UI框架的命名保持一致）
  - `drawStatusBar()` → `renderStatusBar()`（同上）

### 2. 代码结构改进

#### 2.1 函数过长问题
- **文件**: `src/modular/sorter.cpp`
  - `onPhaseChange()` 方法（85行开始）包含多个功能块，建议拆分为更小的方法：
    - `handleScannerPhaseChanges()`
    - `handleOutletPhaseChanges()`
    - `handleReloaderPhaseChanges()`

- **文件**: `src/user_interface/oled.cpp`
  - `displayNormalModeStats()` 方法过长，建议拆分为显示速度统计和显示计数统计两个方法

#### 2.2 参数过多问题
- **文件**: `src/user_interface/oled.h`
  - `displayDashboard()` 方法有5个参数，建议考虑创建一个简单的数据结构或使用多个方法来传递这些参数

#### 2.3 重复代码消除
- **文件**: `src/user_interface/user_interface.cpp`
  - 多个方法都有类似的条件判断：`if (isOutputChannelEnabled(OUTPUT_OLED))` 和 `if (isOutputChannelEnabled(OUTPUT_SERIAL))`
  - 建议创建一个辅助方法 `displayOnEnabledChannels()` 来统一处理这种逻辑

## 三级重构建议（系统架构、类关系、业务逻辑优化）

### 1. 系统架构优化

#### 1.1 采用分层架构
- **当前问题**: 各模块之间存在直接依赖，如 `Sorter` 类直接依赖 `SimpleHMI`
- **改进建议**: 引入三层架构：
  - **表示层**: UserInterface, OLED, Terminal, SimpleHMI
  - **业务逻辑层**: Sorter, DiameterScanner, TrayManager
  - **硬件抽象层**: Encoder, Outlet, Reloader
- **实现方式**: 通过接口或观察者模式实现层间通信，减少直接依赖

#### 1.2 引入事件驱动架构
- **当前问题**: 各模块通过直接调用和标志位通信，耦合度高
- **改进建议**: 实现一个简单的事件系统，模块通过发布-订阅模式通信
- **示例**: 
  ```cpp
  // 事件基类
  class Event {
  public:
      virtual ~Event() = default;
      virtual const char* getName() const = 0;
  };
  
  // 直径测量完成事件
  class DiameterCalculatedEvent : public Event {
  public:
      DiameterCalculatedEvent(int diameter) : diameter(diameter) {}
      const char* getName() const override { return "DiameterCalculated"; }
      int getDiameter() const { return diameter; }
  private:
      int diameter;
  };
  ```

### 2. 类关系优化

#### 2.1 引入依赖注入
- **当前问题**: 几乎所有类都使用单例模式，导致测试困难和紧耦合
- **改进建议**: 为主要类（如 Sorter, DiameterScanner）引入依赖注入
- **示例**: 
  ```cpp
  // 修改前
  class Sorter {
  private:
      Encoder* encoder;
  public:
      Sorter() : encoder(Encoder::getInstance()) {}
  };
  
  // 修改后
  class Sorter {
  private:
      Encoder* encoder;
  public:
      Sorter(Encoder* encoder) : encoder(encoder) {}
  };
  
  // 使用时
  Encoder* encoder = Encoder::getInstance();
  Sorter sorter(encoder);
  ```

#### 2.2 改进单例模式实现
- **当前问题**: 所有单例类的实现不完全一致，有些在构造函数中设置实例指针
- **改进建议**: 使用统一的线程安全单例模式实现
- **示例**: 
  ```cpp
  class SingletonClass {
  private:
      static SingletonClass* instance;
      SingletonClass() {} // 私有构造函数

  public:
      static SingletonClass* getInstance() {
          if (instance == nullptr) {
              instance = new SingletonClass();
          }
          return instance;
      }
  };
  
  // 静态成员初始化
  SingletonClass* SingletonClass::instance = nullptr;
  ```

### 3. 业务逻辑优化

#### 3.1 直径计算算法改进
- **当前问题**: `DiameterScanner::sample()` 方法的直径计算逻辑不够灵活
- **改进建议**: 实现可配置的直径计算策略
- **示例**: 
  ```cpp
  class DiameterCalculationStrategy {
  public:
      virtual ~DiameterCalculationStrategy() = default;
      virtual int calculateDiameter(int* pulseCounts) = 0;
  };
  
  class WeightedAverageStrategy : public DiameterCalculationStrategy {
  public:
      int calculateDiameter(int* pulseCounts) override {
          // 加权平均算法实现
      }
  };
  ```

#### 3.2 速度计算优化
- **当前问题**: `Sorter::getConveyorSpeedPerSecond()` 方法使用简单的时间差计算，容易受到异常值影响
- **改进建议**: 实现滑动窗口平均算法
- **示例**: 
  ```cpp
  float Sorter::getConveyorSpeedPerSecond() {
      static const int WINDOW_SIZE = 5;
      static float speedHistory[WINDOW_SIZE];
      static int historyIndex = 0;
      
      // 计算当前速度
      float currentSpeed = calculateCurrentSpeed();
      
      // 添加到历史记录
      speedHistory[historyIndex] = currentSpeed;
      historyIndex = (historyIndex + 1) % WINDOW_SIZE;
      
      // 计算平均值
      float averageSpeed = 0.0f;
      for (int i = 0; i < WINDOW_SIZE; i++) {
          averageSpeed += speedHistory[i];
      }
      return averageSpeed / WINDOW_SIZE;
  }
  ```

### 4. 代码复用优化

#### 4.1 统一配置管理
- **当前问题**: 配置值分散在各个文件中（如 `SORTER_NUM_OUTLETS`, `UPDATE_INTERVAL`）
- **改进建议**: 创建一个集中的配置类或头文件
- **示例**: 
  ```cpp
  // src/config.h
  #ifndef CONFIG_H
  #define CONFIG_H
  
  // 系统配置
  static const String SYSTEM_NAME = "ESP32 Sorter";
  static const String FIRMWARE_VERSION = "ver: 2601";
  
  // 硬件配置
  static const int NUM_OUTLETS = 8;
  static const int ENCODER_LOGICAL_POSITIONS = 200;
  
  // UI配置
  static const unsigned long DISPLAY_UPDATE_INTERVAL = 2000;
  
  #endif // CONFIG_H
  ```

#### 4.2 统一工具函数
- **当前问题**: 各个类中存在重复的工具函数逻辑
- **改进建议**: 创建一个工具类或头文件
- **示例**: 
  ```cpp
  // src/utils.h
  #ifndef UTILS_H
  #define UTILS_H
  
  #include <Arduino.h>
  
  namespace Utils {
      // 约束值在指定范围内
      template<typename T>
      T constrain(T value, T min, T max) {
          if (value < min) return min;
          if (value > max) return max;
          return value;
      }
      
      // 获取当前时间（毫秒）
      unsigned long getCurrentTime() {
          return millis();
      }
  }
  
  #endif // UTILS_H
  ```

## 实施建议

1. **分阶段实施**: 先实施二级重构，再实施三级重构
2. **单元测试**: 在重构过程中添加单元测试，确保功能不变
3. **代码审查**: 重构后进行代码审查，确保遵循项目规范
4. **性能测试**: 对关键算法（如直径计算、速度计算）进行性能测试

## 预期收益

- **可维护性**: 更清晰的命名和结构，降低理解和维护成本
- **可扩展性**: 分层架构和事件驱动设计，更容易添加新功能
- **可测试性**: 依赖注入和更好的代码结构，更容易编写单元测试
- **性能**: 优化的算法和代码结构，提高系统性能

---

*此文档基于项目当前状态（2026-01-19）生成，具体实施时应根据实际情况调整。*