# ESP32 Sorter 项目规则

## 开发环境
- **主要开发环境**: PlatformIO
- **框架**: Arduino Framework
- **目标板**: ESP32 Dev Module

## 项目结构规范
- 源代码应放在 `src/` 目录下
- 头文件应放在 `include/` 目录下（如需要）
- 库文件应放在 `lib/` 目录下（如需要）
- 配置文件使用 `platformio.ini`

## 命名规范
- 源文件使用小写字母和下划线，如 `main.cpp` 或 `sensor_module.cpp`
- 类和函数使用驼峰命名法
- 常量使用全大写和下划线

## 编译和上传
- 使用PlatformIO IDE或命令行进行编译和上传
- 上传端口需根据实际连接情况在platformio.ini中配置


## 代码风格
- 使用4个空格进行缩进
- 花括号使用K&R风格
- 每行不超过100个字符
- 添加必要的注释说明功能和参数

## 调试
- 使用Serial.println()进行基本调试
- 禁止 JTAG调试器进行调试