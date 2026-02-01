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

// Servo Pins (Outlets 1-8)
constexpr int PIN_SERVO_1 = 13;
constexpr int PIN_SERVO_2 = 12;
constexpr int PIN_SERVO_3 = 14;
constexpr int PIN_SERVO_4 = 27;
constexpr int PIN_SERVO_5 = 26;
constexpr int PIN_SERVO_6 = 25;
constexpr int PIN_SERVO_7 = 33;
constexpr int PIN_SERVO_8 = 32;

// User Interface
constexpr int PIN_BUTTON_MASTER = 4;
constexpr int PIN_BUTTON_SLAVE = 5;

// Power Monitor (Voltage Divider Input)
constexpr int PIN_POWER_MONITOR = 34; // Input Only Pin, good for ADC or Digital Read

// Scanner (4 Points)
constexpr int PIN_SCANNER_1 = 36;
constexpr int PIN_SCANNER_2 = 35;
constexpr int PIN_SCANNER_3 = 34;
constexpr int PIN_SCANNER_4 = 39;

// Encoder
constexpr int PIN_ENCODER_A = 21;
constexpr int PIN_ENCODER_B = 19;
constexpr int PIN_ENCODER_Z = 18;

// Display (I2C)
constexpr int PIN_OLED_SDA = 23;
constexpr int PIN_OLED_SCL = 22;

// ==========================================
// Arrays (Helper for loops)
// ==========================================
constexpr int PINS_SERVO[8] = {
    PIN_SERVO_1, PIN_SERVO_2, PIN_SERVO_3, PIN_SERVO_4,
    PIN_SERVO_5, PIN_SERVO_6, PIN_SERVO_7, PIN_SERVO_8
};

constexpr int PINS_SCANNER[4] = {
    PIN_SCANNER_1, PIN_SCANNER_2, PIN_SCANNER_3, PIN_SCANNER_4
};

// ==========================================
// Hardware Constants
// ==========================================

// Encoder / Conveyor
constexpr int ENCODER_MAX_PHASE = 200;
constexpr int PULSES_PER_TRAY = 200;

// Servos
constexpr int SERVO_POS_CLOSED = 80;
constexpr int SERVO_POS_OPEN = 0;
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

// Legacy Phase Triggers (keeping for compatibility during refactor)
constexpr int PHASE_RESET_OUTLETS_LEGACY = 110;
constexpr int PHASE_CALC_DIAMETER_LEGACY = 120;
constexpr int PHASE_EXECUTE_OUTLETS_LEGACY = 175;

// ==========================================
// EEPROM Addresses
// ==========================================
constexpr int EEPROM_ADDR_DIAMETER = 0;
constexpr int EEPROM_ADDR_SERVO = 0x12; // 18
constexpr int EEPROM_ADDR_MAGIC_SERVO = 0x32; // 50
constexpr int EEPROM_ADDR_BOOT_COUNT = 0x64; // 100 (Allocates 4 bytes)
constexpr int EEPROM_ADDR_TRAY_DATA = 0x70; // 112 (Allocates ~100 bytes for tray array)

// Power Loss Threshold (ADC value: 0-4095)
// Assuming 12V -> 3.3V divider. If 12V drops to 10V, 3.3V drops to ~2.75V.
// 2.75V / 3.3V * 4095 ~= 3412.
// Let's set a safe threshold. User needs to tune this. Defaulting to relatively high drop detection.
// If using digitalRead (HIGH/LOW), this is ignored.
constexpr int POWER_LOSS_ADC_THRESHOLD = 3000;

#endif // CONFIG_H
