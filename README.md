# ESP32 Sorter 项目

这是一个使用PlatformIO和Arduino框架开发的ESP32项目，实现了基本的Hello World功能，作为项目的起点。

## 项目功能
- 启动时打印Hello World和ESP32芯片信息
- 每秒循环打印一次Hello World
- 展示ESP-IDF基本日志系统的使用

## 环境搭建 - PlatformIO（推荐）

### 方法一：VS Code + PlatformIO插件（推荐）

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

### 方法二：命令行安装PlatformIO

```bash
# 安装Python（如果尚未安装）
# 下载并安装Python: https://www.python.org/downloads/

# 安装PlatformIO Core
pip install -U platformio

# 验证安装
pio --version
```

## USB驱动安装（ESP32连接）

- ESP32开发板通常使用CP210x、CH340或FTDI芯片作为USB转串口芯片
- 如果连接ESP32后设备管理器中出现未知设备，需要安装相应驱动：
  - CP210x驱动：https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
  - CH340驱动：https://www.wch.cn/downloads/CH341SER_EXE.html
  - FTDI驱动：https://ftdichip.com/drivers/vcp-drivers/

- 安装驱动后，在设备管理器中查看ESP32连接的COM端口号（如COM3）

### 2. 配置环境变量

#### 使用ESP-IDF Command Prompt（推荐）
- 安装完成后，通过开始菜单找到并打开"ESP-IDF Command Prompt"
- 这个命令提示符已经自动加载了所有必要的环境变量

#### 在普通命令行中设置环境
```powershell
# 假设ESP-IDF安装在默认位置
%userprofile%\esp\esp-idf\export.ps1  # PowerShell
# 或
%userprofile%\esp\esp-idf\export.bat   # 命令提示符(cmd.exe)
```

### 3. 验证ESP-IDF安装
在正确配置环境变量的命令行中执行：
```
idf.py --version
```
应该能看到ESP-IDF的版本信息

### 2. 配置环境变量

#### Windows
- 安装完成后，使用ESP-IDF Command Prompt
- 或者在普通命令行中运行：
```
%userprofile%\.espressif\esp-idf\export.bat
```

#### Linux/MacOS
```
source ~/esp/esp-idf/export.sh
```

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