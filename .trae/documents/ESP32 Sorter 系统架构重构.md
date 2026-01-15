# ESP32 Sorter 系统架构重构计划

## 目标
解耦 OLED 与业务逻辑，合并 UI 模块，创建更清晰的架构

## 重构步骤

### 第一阶段：创建数据结构

1. **创建 `display_data.h`**
   - 定义 `DisplayData` 结构体，包含所有需要显示的数据
   - 包含：模式、速度、直径、托架数量、出口状态、编码器位置等
   - 放在 `src/` 目录下

### 第二阶段：重构 OLED 类

2. **修改 `oled.h`**
   - 移除对 `Encoder` 和 `Sorter` 的直接引用
   - 修改 `update()` 方法签名，接收 `DisplayData` 而不是 `Sorter*`
   - 移除 `encoder` 成员变量

3. **修改 `oled.cpp`**
   - 修改构造函数，不再获取 Encoder 实例
   - 修改 `update()` 方法，从 `DisplayData` 读取数据
   - 修改 `drawEncoderInfo()` 等方法，使用传入的数据
   - 移除对 `encoder->` 和 `sorter->` 的直接调用

### 第三阶段：创建 UserInterface 类

4. **创建 `user_interface.h`**
   - 合并 OLED 和 SimpleHMI 的功能
   - 内部包含 OLED 和 SimpleHMI 实例
   - 提供统一的接口：
     - `initialize()` - 初始化所有 UI 硬件
     - `getDisplayData()` - 获取显示数据结构
     - `updateDisplay(const DisplayData& data)` - 更新显示
     - `isMasterButtonPressed()` - 检查主按钮
     - `isSlaveButtonPressed()` - 检查从按钮
   - 使用单例模式

5. **创建 `user_interface.cpp`**
   - 实现所有接口方法
   - 内部委托给 OLED 和 SimpleHMI
   - 封装临时显示、模式切换等逻辑

### 第四阶段：重构 Sorter 类

6. **修改 `sorter.h`**
   - 移除 `setScannerLogLevel()` 等显示相关方法（如果不需要）
   - 添加 `getDisplayData()` 方法，返回 `DisplayData`
   - 保持核心业务逻辑方法不变

7. **修改 `sorter.cpp`**
   - 实现 `getDisplayData()` 方法
   - 收集所有需要显示的数据到 `DisplayData` 结构体

### 第五阶段：重构主循环

8. **修改 `main.cpp`**
   - 替换 `simpleHMI` 和 `oled` 为 `UserInterface`
   - 在主循环中：
     - 从 UserInterface 获取输入
     - 调用 Sorter 处理业务逻辑
     - 从 Sorter 获取显示数据
     - 通过 UserInterface 更新显示
   - 简化模式切换逻辑

### 第六阶段：清理和测试

9. **删除或标记为废弃**
   - `simple_hmi.h` 和 `simple_hmi.cpp`（功能已合并到 UserInterface）
   - 可以暂时保留，确认新架构稳定后再删除

10. **编译和测试**
    - 编译项目，确保没有错误
    - 测试所有模式的功能
    - 验证显示和输入都正常工作

### 第七阶段：更新文档

11. **更新类关系图**
    - 重新生成 `uml_class_diagram.svg`
    - 反映新的架构：
      - Sorter（核心控制器）
      - UserInterface（UI模块，包含显示和输入）
      - Encoder（单例）
      - DiameterScanner
      - TraySystem
      - Outlet（8个实例）
    - 所有类都通过主循环协调，互不直接依赖

## 预期效果

- ✅ OLED 不再直接依赖 Encoder 和 Sorter
- ✅ UI 模块（显示+输入）统一管理
- ✅ Sorter 专注于业务逻辑
- ✅ 更清晰的职责划分
- ✅ 更容易测试和维护
- ✅ 类关系图更加清晰，没有交叉依赖

## 注意事项

- 重构过程中保持功能不变
- 每个阶段完成后都可以编译测试
- 保留原有代码作为参考，直到新架构稳定