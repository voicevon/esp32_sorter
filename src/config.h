#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// System Information
// ==========================================
constexpr const char* SYSTEM_NAME = "ESP32 Sorter";
constexpr const char* FIRMWARE_VERSION = "2.0.0";

// ==========================================
// Pin Definitions
// ==========================================

// 级联 74HC595 移位寄存器引脚 (控制指示 LED 与 Solenoids H-Bridge)
constexpr int PIN_HC595_DS   = 33;  // 数据输入
constexpr int PIN_HC595_SHCP = 4;   // 移位脉冲
constexpr int PIN_HC595_STCP = 2;   // 锁存脉冲

// HMI Module (Rotary Encoder with Push Button)
constexpr int PIN_HMI_ENC_A = 13;
constexpr int PIN_HMI_ENC_B = 12;
constexpr int PIN_HMI_BTN   = 14;

// Power Monitor (Voltage Divider Input)
constexpr int PIN_POWER_MONITOR = 32;

// Scanner (5 Points Array)
constexpr int PIN_SCANNER_1 = 36;
constexpr int PIN_SCANNER_2 = 39;
constexpr int PIN_SCANNER_3 = 34;
constexpr int PIN_SCANNER_4 = 35;
constexpr int PIN_SCANNER_5 = 15;

// Main Encoder
constexpr int PIN_ENCODER_A = 19;
constexpr int PIN_ENCODER_B = 21;
constexpr int PIN_ENCODER_Z = 18;

// Display (I2C)
constexpr int PIN_OLED_SDA = 23;
constexpr int PIN_OLED_SCL = 22;

// Hardware Serial 0 (Debug)
constexpr int PIN_UART0_TX = 1;
constexpr int PIN_UART0_RX = 3;


constexpr int PINS_SCANNER[5] = {
    PIN_SCANNER_1, PIN_SCANNER_2, PIN_SCANNER_3, PIN_SCANNER_4, PIN_SCANNER_5
};

// ==========================================
// Hardware Constants
// ==========================================

// Encoder / Conveyor
constexpr int ENCODER_MAX_PHASE = 200;
constexpr int PULSES_PER_TRAY = 200;
constexpr bool ENCODER_REVERSE_DIRECTION = false; // 软件反转编码器计数方向

// Output
constexpr int NUM_OUTLETS = 8;

// Scanner
constexpr float SCANNER_WEIGHTS[4] = {1.01f, 1.02f, 1.05f, 1.15f};
constexpr int SCANNER_MIN_DIAMETER_UNIT = 6; // 3mm * 2

// ==========================================
// Timing & Phases
// ==========================================

// Critical Encoder Phases (核心分拣时序相位, 0-199 循环)
// 系统通过编码器追踪物体在分拣流水线上的物理位置，每个循环代表一个托架间隔 (Pitch)

// 1. 启动扫描阶段：托盘进入传感器正下方，直径扫描仪开始记录脉冲宽度
constexpr int PHASE_SCAN_START = 50;

// 2. 数据锁存阶段：物体离开传感器，停止统计脉冲并在此刻计算直径值，推入托盘系统
constexpr int PHASE_DATA_LATCH = PHASE_SCAN_START + 120;  //170

// 3. 执行分级阶段：根据锁存的直径，在流水线出口处匹配对应的分拣仓位，触发电磁铁翻转动作
constexpr int PHASE_OUTLET_EXECUTE = PHASE_SCAN_START - 20;

// 4. 重置归位阶段：在当前托架周期结束前，重置出口控制信号位，清理中间计算标志位，准备下一轮
constexpr int PHASE_OUTLET_RESET = 150;



// ==========================================
// EEPROM Addresses
// ==========================================
constexpr int EEPROM_ADDR_DIAMETER = 0;
constexpr int EEPROM_ADDR_BOOT_COUNT = 0x64; // 100 (Allocates 4 bytes)
constexpr int EEPROM_ADDR_TRAY_DATA = 0x70; // 112 (Allocates ~100 bytes for tray array)

// Power Loss Threshold (ADC value: 0-4095)
constexpr int POWER_LOSS_ADC_THRESHOLD = 3000;

#endif // CONFIG_H
