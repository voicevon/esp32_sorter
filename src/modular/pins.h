// ESP32 Sorter 项目引脚定义文件
#ifndef PINS_H
#define PINS_H

// 舵机引脚定义 - 仅使用ESP32支持的舵机引脚
const int SERVO_PINS[8] = {
    13,   // 出口1舵机
  12,   // 出口2舵机
  14,   // 出口3舵机
  27,   // 出口4舵机
  26,   // 出口5舵机
  25,   // 出口6舵机
  33,   // 出口7舵机
  32    // 出口8舵机
};

// 人机交互模块引脚定义
const int MODE_BUTTON_PIN = 4;     // 主按钮引脚 (Master Button) - 模式切换按钮
const int DIAGNOSTIC_BUTTON_PIN = 5; // 从按钮引脚 (Slave Button)

// 激光扫描仪引脚（4个扫描点）
const int LASER_SCANNER_PINS[4] = {36, 35, 34, 39};  // 激光扫描仪信号输入引脚

// 激光扫描仪权重系数（用于补偿芦笋锥形形状）
const float LASER_SCANNER_WEIGHTS[4] = {1.01, 1.02, 1.05, 1.15};

// 激光扫描仪最小有效直径（单位：unit，1毫米=2个unit）
const int LASER_SCANNER_MIN_DIAMETER = 6;  // 3毫米

// 编码器引脚定义
const int ENCODER_PIN_A = 21;   // 编码器A相引脚
const int ENCODER_PIN_B = 19;   // 编码器B相引脚
const int ENCODER_PIN_Z = 18;   // 编码器Z相（零位）引脚

// SSD1306 I2C显示器引脚定义
const int OLED_SDA_PIN = 23;  // I2C数据引脚
const int OLED_SCL_PIN = 22;  // I2C时钟引脚

// 其他可能的系统引脚定义（可根据实际需要扩展）
const int DEFAULT_SERIAL_BAUD = 115200;

#endif // PINS_H