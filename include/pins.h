// ESP32 Sorter 项目引脚定义文件
#ifndef PINS_H
#define PINS_H

// 舵机引脚定义
const int SERVO_PINS[5] = {
    2,   // 分支器1舵机
    4,   // 分支器2舵机
    5,   // 分支器3舵机
    18,  // 分支器4舵机
    19   // 分支器5舵机
};

// 诊断器引脚定义
const int MODE_BUTTON_PIN = 25;    // 模式切换按钮引脚
const int STATUS_LED1_PIN = 26;    // 状态LED 1引脚
const int STATUS_LED2_PIN = 27;    // 状态LED 2引脚

// 其他可能的系统引脚定义（可根据实际需要扩展）
const int DEFAULT_SERIAL_BAUD = 115200;

#endif // PINS_H