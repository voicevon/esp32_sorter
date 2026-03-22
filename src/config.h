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

// Feeder Outlets (Up to 3, can be digital pins)
constexpr int PIN_FEEDER_1 = 27;
constexpr int PIN_FEEDER_2 = 26;

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

// ==========================================
// Arrays (Helper for loops)
// ==========================================
constexpr int PINS_FEEDER[2] = {
    PIN_FEEDER_1, PIN_FEEDER_2
};

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

// Critical Encoder Phases
constexpr int PHASE_SCAN_START = 1;
constexpr int PHASE_DATA_LATCH = 50;   // Capture scanner data
constexpr int PHASE_OUTLET_EXECUTE = 80; // Outlets move
constexpr int PHASE_OUTLET_RESET = 195;  // Outlets close
constexpr int PHASE_FEEDER_OPEN = 200;
constexpr int PHASE_FEEDER_CLOSE = 220;



// ==========================================
// EEPROM Addresses
// ==========================================
constexpr int EEPROM_ADDR_DIAMETER = 0;
constexpr int EEPROM_ADDR_BOOT_COUNT = 0x64; // 100 (Allocates 4 bytes)
constexpr int EEPROM_ADDR_TRAY_DATA = 0x70; // 112 (Allocates ~100 bytes for tray array)

// Power Loss Threshold (ADC value: 0-4095)
constexpr int POWER_LOSS_ADC_THRESHOLD = 3000;

#endif // CONFIG_H
