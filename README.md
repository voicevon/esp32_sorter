# ESP32 Sorter 项目

这是一个基于ESP32开发的物料分拣系统，使用PlatformIO和Arduino框架开发。系统主要用于传输线上的物料分拣，通过直径数据对物料进行分类，并使用舵机控制的分支器将物料分配到不同通道。

## 项目功能

- 31个传输线托架的环形管理系统
- 5个分支器舵机控制（用于物料分配）
- 单点扫描仪直径数据接收和处理
- 基于物料直径的智能分类算法
- 异常情况处理和系统自检功能
- 集成测试模块，验证系统各组件功能
- 串口命令控制，便于调试和监控

## 项目架构

- **Carriage**：表示传输线上的单个托架
- **CarriageManager**：管理所有托架的状态和移动
- **DiverterController**：控制5个分支器舵机的动作
- **SorterController**：系统主控协调器，协调各组件工作

## 目录结构

```
d:\Firmware\esp32_sorter\
├── include/              # 头文件目录
│   ├── carriage_system.h         # 传输系统相关头文件
│   ├── diverter_controller.h     # 分支器控制头文件
│   ├── sorter_controller.h       # 系统主控协调器头文件
│   └── system_integration_test.h # 系统集成测试头文件
├── src/                  # 源代码目录
│   ├── carriage_system.cpp       # 传输系统实现
│   ├── diverter_controller.cpp   # 分支器控制实现
│   ├── sorter_controller.cpp     # 系统主控协调器实现
│   ├── system_integration_test.cpp # 系统集成测试实现
│   └── main.cpp                  # 主程序入口
└── platformio.ini        # PlatformIO配置文件
```

## 环境搭建 - PlatformIO

### 方法：VS Code + PlatformIO插件（推荐）

1. **安装VS Code**
   - 从官网下载并安装：https://code.visualstudio.com/
   - 安装完成后打开VS Code

2. **安装PlatformIO插件**
   - 点击左侧扩展图标（或按Ctrl+Shift+X）
   - 搜索"PlatformIO IDE"
   - 点击安装，等待安装完成
   - 安装完成后重启VS Code

3. **打开项目**
   - 在VS Code中点击"文件" -> "打开文件夹"
   - 选择项目目录 `d:/Firmware/esp32_sorter`
   - PlatformIO会自动识别项目并加载相关配置

## USB驱动安装（ESP32连接）

- ESP32开发板通常使用CP210x、CH340或FTDI芯片作为USB转串口芯片
- 如果连接ESP32后设备管理器中出现未知设备，需要安装相应驱动：
  - CP210x驱动：https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
  - CH340驱动：https://www.wch.cn/downloads/CH341SER_EXE.html
  - FTDI驱动：https://ftdichip.com/drivers/vcp-drivers/

- 安装驱动后，在设备管理器中查看ESP32连接的COM端口号（如COM3）

## 使用PlatformIO构建和烧录项目

### 使用VS Code + PlatformIO插件

1. **配置端口**
   - 打开项目中的 `platformio.ini` 文件
   - 将 `upload_port` 和 `monitor_port` 改为你实际的COM端口（如COM3）

2. **构建项目**
   - 点击VS Code底部状态栏中的 ✓ 图标（Build）
   - 或按F7键

3. **烧录项目**
   - 点击底部状态栏中的 → 图标（Upload）
   - 或按Ctrl+Alt+U键

4. **监控串口输出**
   - 点击底部状态栏中的 🔌 图标（Serial Monitor）
   - 或按Ctrl+Alt+S键

## 测试模式

系统启动后会自动运行自检和集成测试，然后进入测试模式。在测试模式下：

- 系统会生成模拟的直径数据
- 控制传输线移动
- 演示物料分类和分配过程

## 串口命令控制

可以通过串口监视器发送以下命令：

- `start`: 启动系统
- `stop`: 停止系统
- `test`: 进入测试模式
- `status`: 显示系统状态
- `reset`: 重置系统

## 代码风格规范

- 使用4个空格进行缩进
- 花括号使用K&R风格
- 每行不超过100个字符
- 添加必要的注释说明功能和参数
- 类和函数使用驼峰命名法
- 常量使用全大写和下划线

## 调试

- 使用Serial.println()进行基本调试
- 系统配置了调试日志级别，可以在platformio.ini中调整
- 禁止使用JTAG调试器进行调试

## 故障排除

- 如果构建失败，请检查include目录配置是否正确
- 如果上传失败，请确认COM端口配置和USB连接
- 如果系统运行异常，可以通过串口监视器查看调试信息   - 或按Ctrl+Alt+U键

4. **监控串口输出**
   - 点击底部状态栏中的 ![Serial Monitor] 图标
   - 或按Ctrl+Alt+S键

### 使用命令行

1. **进入项目目录**
```bash
cd d:/Firmware/esp32_sorter
```

2. **构建项目**
```bash
pio run
```

3. **烧录项目**（确保修改platformio.ini中的端口配置）
```bash
pio run -t upload
```

4. **监控串口**
```bash
pio device monitor
```

## 项目结构
```
esp32_sorter/
├── .trae/
│   └── rules/
│       └── project_rules.md    # 项目规则
├── src/
│   └── main.cpp                 # 主程序源文件（Arduino框架）
├── platformio.ini              # PlatformIO配置文件
├── README.md                   # 项目说明文档
└── CMakeLists.txt              # ESP-IDF配置文件（保留用于兼容性）
```

## 常见问题解决

- **端口错误**：
  1. 确保在platformio.ini中正确设置了COM端口
  2. 检查USB驱动是否正确安装
  3. 尝试更换USB线缆或USB端口

- **烧录失败**：
  1. 确保ESP32开发板已正确连接
  2. 某些ESP32开发板可能需要在烧录时按住BOOT键
  3. 检查是否有其他程序占用了串口

- **编译错误**：
  1. 确保PlatformIO已正确安装
  2. 尝试在VS Code中点击"PlatformIO: Rebuild IntelliSense Index"
  3. 检查代码语法是否正确

## 下一步开发

1. 熟悉Arduino框架API文档
2. 根据项目需求添加传感器或执行器代码
3. 使用PlatformIO的库管理器添加必要的依赖库

## 项目结构
```
esp32_sorter/
├── main.c                # 主程序源文件
├── CMakeLists.txt        # 项目级CMake配置
├── main/                 # 主组件目录
│   └── CMakeLists.txt    # 组件级CMake配置
└── README.md             # 项目说明文档
```

## 注意事项
- 确保ESP32开发板已正确连接到计算机
- 使用正确的串口进行烧录
- 如有编译错误，请检查ESP-IDF环境是否正确安装