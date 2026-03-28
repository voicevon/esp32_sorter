#include "shift_register_driver.h"

ShiftRegisterDriver::ShiftRegisterDriver(int ds, int shcp, int stcp) 
    : lastShiftData(0xFFFFFFFF), dsPin(ds), shcpPin(shcp), stcpPin(stcp) {
}

void ShiftRegisterDriver::initialize() {
    pinMode(dsPin, OUTPUT);
    pinMode(shcpPin, OUTPUT);
    pinMode(stcpPin, OUTPUT);
    digitalWrite(stcpPin, LOW);
}

void ShiftRegisterDriver::write(uint8_t chip2, uint8_t chip1, uint8_t led) {
    uint32_t currentData = ((uint32_t)chip2 << 16) | ((uint32_t)chip1 << 8) | led;
    
    if (currentData != lastShiftData) {
        forceUpdate(chip2, chip1, led);
        lastShiftData = currentData;
    }
}

void ShiftRegisterDriver::forceUpdate(uint8_t chip2, uint8_t chip1, uint8_t led) {
    digitalWrite(stcpPin, LOW);
    shiftOut(dsPin, shcpPin, MSBFIRST, chip2);
    shiftOut(dsPin, shcpPin, MSBFIRST, chip1);
    shiftOut(dsPin, shcpPin, MSBFIRST, led);
    digitalWrite(stcpPin, HIGH);
    
    // 同步记录，以防后续 write() 冗余
    lastShiftData = ((uint32_t)chip2 << 16) | ((uint32_t)chip1 << 8) | led;
}
