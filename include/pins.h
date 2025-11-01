// ESP32 Sorter 项目引脚定义文件
#ifndef PINS_H
#define PINS_H

// 舵机引脚定义 - 仅使用ESP32支持的舵机引脚
const int SERVO_PINS[5] = {
    2,    // 分支器1舵机
    4,    // 分支器2舵机
    16,   // 分支器3舵机
    17,   // 分支器4舵机
    5     // 分支器5舵机
};

// 诊断器引脚定义
const int MODE_BUTTON_PIN = 33;    // 模式切换按钮引脚
const int DIAGNOSTIC_BUTTON_PIN = 32; // 诊断按钮引脚
const int STATUS_LED1_PIN = 27;    // 状态LED 1引脚 (从35修改为27，因为35是输入专用)
const int STATUS_LED2_PIN = 26;    // 状态LED 2引脚 (从34修改为26，因为34是输入专用)

// 激光扫描仪引脚
const int LASER_SCANNER_PIN = 13;  // 激光扫描仪信号输入引脚

// 编码器引脚定义
const int ENCODER_PIN_A = 21;   // 编码器A相引脚
const int ENCODER_PIN_B = 19;   // 编码器B相引脚
const int ENCODER_PIN_Z = 18;   // 编码器Z相（零位）引脚

// 其他可能的系统引脚定义（可根据实际需要扩展）
const int DEFAULT_SERIAL_BAUD = 115200;

#endif // PINS_H