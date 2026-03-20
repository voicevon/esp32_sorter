#include "modbus_controller.h"
#include "../config.h"

unsigned long lastModbusSendTime = 0;

void ModbusController::initialize() {
    pinMode(PIN_RS485_EN, OUTPUT);
    digitalWrite(PIN_RS485_EN, LOW);
    Serial1.begin(MODBUS_BAUD_RATE, MODBUS_SERIAL_CONFIG, PIN_RS485_RX, PIN_RS485_TX);
    Serial.println("[Modbus] Controller Initialized.");
}

uint16_t ModbusController::calculateCRC16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)data[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) { crc >>= 1; crc ^= 0xA001; }
            else                      { crc >>= 1; }
        }
    }
    return crc;
}

// ─────────────────────────────────────────────
// 写寄存器 (同步发送，约 1ms，无阻塞等待)
// ─────────────────────────────────────────────
void ModbusController::writeRegister(uint16_t regAddr, uint16_t regValue) {
    uint8_t frame[8];
    frame[0] = MODBUS_SERVO_SLAVE_ID;
    frame[1] = 0x06;
    frame[2] = (regAddr  >> 8) & 0xFF;
    frame[3] =  regAddr        & 0xFF;
    frame[4] = (regValue >> 8) & 0xFF;
    frame[5] =  regValue       & 0xFF;
    uint16_t crc = calculateCRC16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;

    digitalWrite(PIN_RS485_EN, HIGH);
    Serial1.write(frame, 8);
    Serial1.flush();
    digitalWrite(PIN_RS485_EN, LOW);
    lastModbusSendTime = millis();
}

// ─────────────────────────────────────────────
// 异步读: Step 1 — 发出读请求 (立即返回)
// ─────────────────────────────────────────────
void ModbusController::requestRegisterRead(uint16_t regAddr) {
    // 清空上一次的残留数据
    while (Serial1.available()) Serial1.read();

    uint8_t frame[8];
    frame[0] = MODBUS_SERVO_SLAVE_ID;
    frame[1] = 0x03;
    frame[2] = (regAddr >> 8) & 0xFF;
    frame[3] =  regAddr       & 0xFF;
    frame[4] = 0x00;
    frame[5] = 0x01;
    uint16_t crc = calculateCRC16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;

    digitalWrite(PIN_RS485_EN, HIGH);
    Serial1.write(frame, 8);
    Serial1.flush();
    digitalWrite(PIN_RS485_EN, LOW);

    // 启动异步等待状态
    _rxCount   = 0;
    _rxPending = true;
    _rxStartMs = millis();
}

// ─────────────────────────────────────────────
// 异步读: Step 2 — 在 loop() 中轮询 (非阻塞)
// 返回 true  → outValue 已填充，数据有效
// 返回 false → 数据尚未到达 / 已超时
// ─────────────────────────────────────────────
bool ModbusController::pollReadResult(uint16_t &outValue) {
    if (!_rxPending) return false;

    // 读取已到达的字节（不等待）
    while (Serial1.available() && _rxCount < 7) {
        _rxBuf[_rxCount++] = Serial1.read();
    }

    if (_rxCount >= 7) {
        _rxPending = false;
        if (_rxBuf[0] != MODBUS_SERVO_SLAVE_ID || _rxBuf[1] != 0x03) {
            outValue = 0xFFFE; // 响应帧错误
            return true;
        }
        outValue = ((uint16_t)_rxBuf[3] << 8) | _rxBuf[4];
        return true;
    }

    // 超时判断
    if (millis() - _rxStartMs > RX_TIMEOUT_MS) {
        _rxPending = false;
        outValue = 0xFFFF; // 超时，无响应
        return true;
    }

    return false; // 数据还没到，本次 loop() 跳过
}

// ─────────────────────────────────────────────
// 废弃的阻塞读 (仅供配置写入等低频场景使用)
// ─────────────────────────────────────────────
uint16_t ModbusController::readRegisterSync(uint16_t regAddr) {
    requestRegisterRead(regAddr);
    uint32_t start = millis();
    while (millis() - start < RX_TIMEOUT_MS) {
        uint16_t val;
        if (pollReadResult(val)) return val;
    }
    return 0xFFFF;
}
