# 扫描仪诊断逻辑重构分析

## 当前代码结构

### main.cpp中的逻辑（第429-473行）
- 模式初始化（首次进入或切换回该模式时）
- 子模式切换处理（使用按钮切换）
- 根据子模式执行相应功能
- 调用ScannerDiagnosticHandler的方法

### ScannerDiagnosticHandler类的功能
- 封装DiameterScanner的基本操作
- 管理编码器值记录模式
- 提供显示IO状态、原始直径的方法
- 提供获取编码器值的接口

## 优缺点分析

### 将逻辑放在main.cpp中的优缺点

**优点：**
- 主控制流程清晰直观，便于理解整个系统的运行流程
- 子模式切换逻辑集中在一处，易于调试和跟踪
- 对外部组件的调用关系明确，便于理解各模块间的交互
- 不需要额外设计类接口，实现简单直接

**缺点：**
- main.cpp职责过重，包含了太多具体业务逻辑，违背了单一职责原则
- ScannerDiagnosticHandler类功能不完整，只做了简单封装，未能充分发挥面向对象的优势
- 代码耦合度高，修改一处可能影响多处，维护成本高
- 不利于扩展，比如添加新的子模式需要修改main.cpp
- 代码复用性差，相似逻辑无法在其他地方重用

### 将逻辑移到ScannerDiagnosticHandler类中的优缺点

**优点：**
- 符合单一职责原则：ScannerDiagnosticHandler完全负责扫描仪诊断模式的所有功能
- 封装性更好：隐藏内部实现细节，只暴露必要接口给外部
- 便于维护：所有相关逻辑集中在一个类中，修改时不需要在多个文件间跳转
- 可扩展性强：添加新子模式或修改现有逻辑只需修改类内部，不影响main.cpp
- 代码复用性提高，相关功能可以在其他地方轻松调用
- main.cpp更简洁，只负责系统级的状态管理和调用

**缺点：**
- 可能增加类的复杂度，需要合理设计内部结构
- 主控制流程可能不如直接在main.cpp中直观
- 需要设计好类的接口，确保与main.cpp的交互清晰
- 重构需要一定的时间和测试成本

## 重构建议

### 1. 增强ScannerDiagnosticHandler类

在`scanner_diagnostic_handler.h`中添加以下功能：
- 子模式管理变量（currentSubMode, lastSubMode）
- 模式初始化方法
- 子模式切换和处理方法
- 按钮输入处理方法

### 2. 重构后的类接口示例

```cpp
class ScannerDiagnosticHandler {
private:
    // 现有成员变量
    DiameterScanner* scanner;
    bool scanEdgeCalibrationMode;
    
    // 新增成员变量
    int currentSubMode;
    int lastSubMode;
    bool isInitialized;
    
public:
    // 现有方法保持不变
    ScannerDiagnosticHandler();
    void initialize();
    void displayIOStatus();
    void displayRawDiameters();
    void enableScanEdgeCalibration();
    void disableScanEdgeCalibration();
    const int* getRisingEdgeEncoderValues() const;
    const int* getFallingEdgeEncoderValues() const;
    void onPhaseChange(int phase);
    void processTasks();
    
    // 新增方法
    void activate();  // 激活诊断模式
    void deactivate();  // 停用诊断模式
    void update();  // 主更新方法，处理所有诊断逻辑
    void handleSlaveButtonPress();  // 处理从按钮输入
    int getCurrentSubMode() const;  // 获取当前子模式
};
```

### 3. main.cpp中的简化

重构后，main.cpp中的代码将简化为：

```cpp
case MODE_DIAGNOSE_SCANNER:
    // 激活诊断模式（如果尚未激活）
    scannerDiagnosticHandler.activate();
    
    // 处理按钮输入
    if (userInterface->isSlaveButtonPressed()) {
        scannerDiagnosticHandler.handleSlaveButtonPress();
    }
    
    // 执行诊断模式的主要逻辑
    scannerDiagnosticHandler.update();
    
    // 处理编码器相关任务
    if (scannerDiagnosticHandler.getCurrentSubMode() == 1 || 
        scannerDiagnosticHandler.getCurrentSubMode() == 2) {
        scannerDiagnosticHandler.processTasks();
    }
    break;
```

### 4. 实现细节建议

- 将子模式切换逻辑移到`handleSlaveButtonPress()`方法中
- 将模式初始化逻辑移到`activate()`方法中
- 将根据子模式执行功能的逻辑移到`update()`方法中
- 使用私有辅助方法来组织复杂逻辑，保持公共接口简洁

## 结论

将扫描仪诊断逻辑移到ScannerDiagnosticHandler类中是更好的选择，因为它符合面向对象的设计原则，提高了代码的可维护性、可扩展性和复用性。虽然重构需要一定的时间和测试成本，但从长期来看，这种结构更有利于项目的发展和维护。

重构后的代码将更加模块化，各组件职责明确，便于团队协作和未来功能扩展。