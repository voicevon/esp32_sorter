# ESP32 Sorter 项目

这是一个基于ESP32开发的物料分拣系统，使用PlatformIO和Arduino框架开发。系统主要用于传输线上的物料分拣，通过直径数据对物料进行分类，并使用舵机控制的出口将物料分配到不同通道。

## 项目功能

- 31个传输线托盘的环形管理系统
- 8个出口舵机控制（用于物料分配）
- 4点激光扫描仪直径数据接收和处理
- 基于物料直径的智能分类算法（使用中位数算法）
- 编码器相位触发的采样和控制
- 多种诊断模式支持
- 上料器控制功能
- 串口调试日志输出

## 项目架构

- **Sorter**：系统主控协调器，协调各组件工作
- **TraySystem**：管理所有托盘的状态和移动
- **Encoder**：编码器类，提供位置跟踪和中断处理
- **Outlet**：单个出口的控制
- **SimpleHMI**：人机交互模块，处理按钮和LED
- **DiameterScanner**：直径扫描仪类，处理激光扫描数据

## 目录结构

```
d:\Firmware\esp32_sorter\
├── .trae/              # 项目规则目录
│   └── rules/
│       └── project_rules.md
├── doc/                # 文档目录
├── src/                # 源代码目录
│   ├── main.cpp           # 主程序入口
│   ├── sorter.cpp         # 系统主控协调器实现
│   ├── sorter.h           # 系统主控协调器头文件
│   ├── tray_system.cpp    # 传输系统实现
│   ├── tray_system.h      # 传输系统头文件
│   ├── outlet.h           # 出口控制头文件
│   ├── encoder.cpp        # 编码器实现
│   ├── encoder.h          # 编码器头文件
│   ├── simple_hmi.cpp     # 人机交互模块实现
│   ├── simple_hmi.h       # 人机交互模块头文件
│   ├── diameter_scanner.cpp # 直径扫描仪实现
│   ├── diameter_scanner.h   # 直径扫描仪头文件
│   └── pins.h             # 引脚定义文件
├── .gitignore           # Git忽略文件
├── CMakeLists.txt       # CMake配置文件
├── README.md            # 项目说明文档
├── compile_commands.json # 编译命令文件
└── platformio.ini       # PlatformIO配置文件
```

## 环境搭建

### VS Code + PlatformIO插件（推荐）

1. 安装VS Code
2. 安装PlatformIO插件
3. 打开项目目录 `d:/Firmware/esp32_sorter`
4. PlatformIO会自动识别项目并加载相关配置

### USB驱动安装

ESP32开发板通常使用CP210x、CH340或FTDI芯片作为USB转串口芯片，需要安装相应驱动。

## 编译和烧录

### 使用VS Code + PlatformIO插件

1. **构建项目**：点击VS Code底部状态栏中的 ✓ 图标或按F7键
2. **烧录项目**：点击底部状态栏中的 → 图标或按Ctrl+Alt+U键
3. **监控串口**：点击底部状态栏中的 🔌 图标或按Ctrl+Alt+S键

### 使用命令行

```bash
# 构建项目
C:\Users\feng\.platformio\penv\Scripts\platformio.exe run

# 烧录项目
C:\Users\feng\.platformio\penv\Scripts\platformio.exe run -t upload
```

## 系统模式

系统支持多种工作模式：

- **MODE_NORMAL**：正常工作模式
- **MODE_DIAGNOSE_ENCODER**：诊断编码器模式
- **MODE_DIAGNOSE_SCANNER**：诊断扫描仪模式
- **MODE_DIAGNOSE_OUTLET**：诊断出口模式
- **MODE_DIAGNOSE_CONVEYOR**：诊断传输线模式
- **MODE_TEST**：测试模式
- **MODE_TEST_RELOADER**：上料器测试模式

使用主按钮可以切换不同的工作模式。

## 代码风格规范

- 使用4个空格进行缩进
- 花括号使用K&R风格
- 每行不超过100个字符
- 添加必要的注释说明功能和参数
- 类和函数使用驼峰命名法
- 常量使用全大写和下划线

## 调试

- 使用Serial.println()进行基本调试
- 禁止使用JTAG调试器进行调试

## 常见问题解决

- **端口错误**：确保在platformio.ini中正确设置了COM端口
- **烧录失败**：检查ESP32开发板连接，某些开发板可能需要在烧录时按住BOOT键
- **编译错误**：确保PlatformIO已正确安装，检查代码语法

## 注意事项

- 系统仅使用串口进行数据输出（调试日志），不会从外部接收串口命令
- 系统控制通过诊断控制器按钮实现
- 编码器使用AB相双中断模式和四状态解码算法
- 禁止创建和使用Tray、TrayManager、OutletController类

## 故障排除

- 如果构建失败，请检查代码语法和依赖库
- 如果上传失败，请确认COM端口配置和USB连接
- 如果系统运行异常，可以通过串口监视器查看调试信息