// ESP32 Sorter 项目引脚定义文件
#ifndef PINS_H
#define PINS_H

// 舵机引脚定义 - 仅使用ESP32支持的舵机引脚
const int SERVO_PINS[5] = {
    2,    // 出口1舵机
  4,    // 出口2舵机
  16,   // 出口3舵机
  17,   // 出口4舵机
  5     // 出口5舵机
};

// 上料器舵机引脚
const int RELOADER_SERVO_PIN = 15;  // 上料器舵机控制引脚

// 人机交互模块引脚定义
const int MODE_BUTTON_PIN = 27;    // 主按钮引脚 (Master Button)
const int DIAGNOSTIC_BUTTON_PIN = 12; // 从按钮引脚 (Slave Button)
const int STATUS_LED1_PIN = 14;    // 主LED引脚 (Master LED)
const int STATUS_LED2_PIN = 13;    // 从LED引脚 (Slave LED)

// 激光扫描仪引脚
const int LASER_SCANNER_PIN = 32;  // 激光扫描仪信号输入引脚

// 编码器引脚定义
const int ENCODER_PIN_A = 19;   // 编码器A相引脚
const int ENCODER_PIN_B = 21;   // 编码器B相引脚
const int ENCODER_PIN_Z = 18;   // 编码器Z相（零位）引脚

// 其他可能的系统引脚定义（可根据实际需要扩展）
const int DEFAULT_SERIAL_BAUD = 115200;

#endif // PINS_H