#include "modbus_controller.h"
#include "../config.h"

// 全局变量映射（为了兼容性或内部使用）
unsigned long lastModbusSendTime = 0;

void ModbusController::initialize() {
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, LOW); // 默认接收模式
    Serial1.begin(MODBUS_BAUD_RATE, MODBUS_SERIAL_CONFIG, PIN_RS485_RX, PIN_RS485_TX);
    Serial.println("[Modbus] Controller Initialized.");
}

uint16_t ModbusController::calculateCRC16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)data[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void ModbusController::writeRegister(uint16_t regAddr, uint16_t regValue) {
    uint8_t frame[8];
    frame[0] = MODBUS_SERVO_SLAVE_ID;
    frame[1] = 0x06;
    frame[2] = (regAddr >> 8) & 0xFF;
    frame[3] = regAddr & 0xFF;
    frame[4] = (regValue >> 8) & 0xFF;
    frame[5] = regValue & 0xFF;

    uint16_t crc = calculateCRC16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;

    Serial.print("[Modbus TX] ");
    for(int i=0; i<8; i++) Serial.printf("%02X ", frame[i]);
    Serial.println();

    digitalWrite(PIN_RS485_EN, HIGH);
    Serial1.write(frame, 8);
    Serial1.flush();
    digitalWrite(PIN_RS485_EN, LOW);
    
    lastModbusSendTime = millis();
}

void ModbusController::requestRegisterRead(uint16_t regAddr) {
    uint8_t frame[8];
    frame[0] = MODBUS_SERVO_SLAVE_ID;
    frame[1] = 0x03;
    frame[2] = (regAddr >> 8) & 0xFF;
    frame[3] = regAddr & 0xFF;
    frame[4] = 0x00;
    frame[5] = 0x01; // 读取 1 个寄存器

    uint16_t crc = calculateCRC16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;

    digitalWrite(PIN_RS485_EN, HIGH);
    Serial1.write(frame, 8);
    Serial1.flush();
    digitalWrite(PIN_RS485_EN, LOW);
}

// 同步读取寄存器 (阻塞式，带超时)
uint16_t ModbusController::readRegisterSync(uint16_t regAddr) {
    // 强制清理接收槽位，排除残余干扰
    while (Serial1.available()) Serial1.read();
    delay(10); // 呼吸间隔

    requestRegisterRead(regAddr);
    
    unsigned long startTime = millis();
    uint8_t buffer[7]; // SlaveID(1) + CMD(1) + Len(1) + Value(2) + CRC(2)
    int bytesRead = 0;
    
    // 增加一点等待时间，驱动器响应可能存在滞后
    while (millis() - startTime < 150) { 
        if (Serial1.available()) {
            buffer[bytesRead++] = Serial1.read();
            if (bytesRead >= 7) break;
        }
        yield(); // 给 ESP32 其它后台任务留点时间
    }
    
    if (bytesRead < 7) {
        // Serial.printf("[Modbus ERR] Read Timeout at 0x%04X, bytes: %d\n", regAddr, bytesRead);
        return 0xFFFF; // 读取失败
    }
    
    // 校验响应头
    if (buffer[0] != MODBUS_SERVO_SLAVE_ID || buffer[1] != 0x03) return 0xFFFE;
    
    uint16_t value = (buffer[3] << 8) | buffer[4];
    return value;
}

