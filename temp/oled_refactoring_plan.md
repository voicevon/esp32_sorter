# OLED类重构方案：移除SystemMode依赖

## 1. 重构目标
- 将OLED类从依赖于系统工作模式（SystemMode）的组件转变为通用的底层显示组件
- 方法命名基于功能而非模式，提高代码的可复用性和灵活性
- 移除硬编码的模式依赖，实现更好的代码解耦
- 允许不同模式共享相同的显示界面，提高代码复用率

## 2. 当前依赖分析

### 2.1 OLED类中直接使用SystemMode的地方

| 方法签名 | 依赖类型 | 用途 |
|---------|---------|------|
| `void drawSystemInfo(SystemMode currentMode)` | 直接使用 | 显示系统模式信息 |
| `void update(const DisplayData& data)` | 间接使用 | 通过DisplayData结构体中的currentMode字段进行条件判断 |
| `void displayModeChange(SystemMode newMode)` | 直接使用 | 显示模式变化信息 |
| `DisplayData lastDisplayData` | 间接使用 | 存储上一次显示的DisplayData，包含SystemMode字段 |

### 2.2 模式专用方法

| 方法签名 | 用途 |
|---------|------|
| `void displayNormalMode(const DisplayData& data)` | 显示正常模式信息 |
| `void displayEncoderDiagnosticMode(const DisplayData& data)` | 显示编码器诊断模式信息 |
| `void displayOutletDiagnosticMode(const DisplayData& data)` | 显示出口诊断模式信息 |
| `void displayOtherModes(const DisplayData& data)` | 显示其他模式信息 |

### 2.3 DisplayData结构体的影响

`DisplayData`结构体包含`SystemMode currentMode`字段，OLED类通过这个结构体间接依赖SystemMode。

## 3. 重构方案

### 3.1 方法重命名与重构

| 当前方法 | 重构后方法 | 功能说明 |
|---------|---------|---------|
| `void drawSystemInfo(SystemMode currentMode)` | `void drawStatusBar(const String& statusText)` | 绘制状态栏，显示任意状态文本而非固定模式名称 |
| `void displayNormalMode(const DisplayData& data)` | 移除，使用新的功能专用方法 | 拆分为多个功能专用方法 |
| `void displayEncoderDiagnosticMode(const DisplayData& data)` | `void displayPositionInfo(const String& title, int position, bool showOnlyOnChange)` | 显示位置信息，可用于编码器或其他需要显示位置的场景 |
| `void displayOutletDiagnosticMode(const DisplayData& data)` | 移除，使用已有的`displayOutletTestGraphic` | 已有的方法已经足够通用 |
| `void displayOtherModes(const DisplayData& data)` | 移除，由调用者选择合适的显示方法 | 不再需要，调用者直接调用功能方法 |
| `void displayModeChange(SystemMode newMode)` | `void displayModeChange(const String& newModeName)` | 显示模式变化，使用模式名称字符串而非SystemMode枚举 |
| `void update(const DisplayData& data)` | `void updateDisplay()` | 移除对DisplayData和SystemMode的依赖，由调用者直接调用功能方法 |

### 3.2 新增功能专用方法

| 方法签名 | 功能说明 |
|---------|---------|
| `void displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount)` | 显示速度统计信息 |
| `void displaySingleValue(const String& label, int value, const String& unit)` | 显示单个值（如直径、温度等） |
| `void displayDiagnosticValues(const String& title, const String& value1, const String& value2)` | 显示诊断相关的两个值 |
| `void displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3 = "")` | 显示多行文本信息 |

### 3.3 移除DisplayData依赖

- 移除OLED类中对`DisplayData`结构体的直接依赖
- 不再存储`lastDisplayData`，改为根据需要检查数据变化
- 调用者直接传递所需的显示参数，而非整个DisplayData对象

### 3.4 重构后的OLED类接口

```cpp
class OLED {
private:
    // ... 私有成员变量 ...
    
    // 私有辅助方法
    void drawHeader();
    void drawStatusBar(const String& statusText);
    void checkTemporaryDisplayEnd();
    
public:
    static OLED* getInstance();
    bool isAvailable() const;
    void initialize();
    
    // 通用显示方法
    void displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount);
    void displaySingleValue(const String& label, int value, const String& unit);
    void displayPositionInfo(const String& title, int position, bool showOnlyOnChange);
    void displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode);
    void displayDiagnosticValues(const String& title, const String& value1, const String& value2);
    void displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3 = "");
    
    // 状态显示方法
    void displayModeChange(const String& newModeName);
    void displayOutletStatus(uint8_t outletIndex, bool isOpen);
    void displayDiagnosticInfo(const String& title, const String& info);
    
    // 特殊功能方法
    void displayScannerEncoderValues(const int* risingValues, const int* fallingValues);
    void resetDiagnosticMode();
};
```

## 4. 实现步骤

### 4.1 第一步：创建新的功能专用方法
- 实现新增的功能专用方法（如`displaySpeedStats`、`displaySingleValue`等）
- 确保这些方法不依赖于SystemMode或DisplayData

### 4.2 第二步：修改现有方法
- 重命名并重构现有方法，移除SystemMode依赖
- 更新方法实现，使用新的功能专用方法

### 4.3 第三步：移除SystemMode和DisplayData依赖
- 移除OLED类中对SystemMode的直接引用
- 移除对DisplayData结构体的依赖

### 4.4 第四步：更新调用者
- 修改UserInterface类，使用新的OLED类接口
- 修改其他调用OLED类的地方，使用功能专用方法

### 4.5 第五步：测试与验证
- 确保所有显示功能正常工作
- 验证不同模式下的显示内容正确

## 5. 预期效果

### 5.1 代码结构改进
- OLED类不再依赖SystemMode，成为通用显示组件
- 方法命名基于功能，提高代码可读性和可维护性
- 减少硬编码依赖，提高代码灵活性

### 5.2 功能改进
- 不同模式可以共享相同的显示方法
- 更容易添加新的显示功能
- 提高代码复用率

### 5.3 可维护性改进
- 降低模块间的耦合度
- 更容易理解和修改显示逻辑
- 便于进行单元测试

## 6. 潜在风险与应对措施

### 6.1 兼容性风险
- **风险**：重构可能影响现有功能
- **应对措施**：逐步实现重构，确保每个阶段都能正常工作；保留旧方法作为包装器，逐步替换

### 6.2 性能影响
- **风险**：移除数据缓存可能导致频繁更新
- **应对措施**：在调用者层面实现数据变化检测，只在数据变化时调用显示方法

### 6.3 代码复杂度增加
- **风险**：需要更多的方法调用和参数传递
- **应对措施**：保持方法接口简洁，提供合理的默认参数

## 7. 总结

通过本次重构，OLED类将从一个依赖于系统模式的组件转变为通用的底层显示组件，方法命名基于功能而非模式，提高了代码的可复用性和灵活性。重构后，不同的系统模式可以根据需要选择合适的显示方法，实现了更好的代码解耦和模块化设计。