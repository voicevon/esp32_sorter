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
// 内部写实现 (直接操作总线，无仲裁)
// ─────────────────────────────────────────────
void ModbusController::_doWrite(uint16_t regAddr, uint16_t regValue) {
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
// 写寄存器 (含总线仲裁)
// 若当前有读请求正在飞行，写操作被缓冲至写队列，
// pollReadResult() 完成后立即执行。
// ─────────────────────────────────────────────
void ModbusController::writeRegister(uint16_t regAddr, uint16_t regValue) {
    if (_rxPending) {
        // 读请求还在飞行中 —— 缓冲写操作（后写覆盖前写，对速度指令安全）
        _pendingWriteAddr  = regAddr;
        _pendingWriteValue = regValue;
        _pendingWriteValid = true;
        return;
    }
    _doWrite(regAddr, regValue);
}

// ─────────────────────────────────────────────
// 异步读: Step 1 — 发出读请求 (立即返回)
// ─────────────────────────────────────────────
void ModbusController::requestRegisterRead(uint16_t regAddr) {
    while (Serial1.available()) Serial1.read(); // 清残留

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

    _rxCount   = 0;
    _rxPending = true;
    _rxStartMs = millis();
}

// ─────────────────────────────────────────────
// 异步读: Step 2 — 轮询结果 (非阻塞)
// 返回 true  → outValue 已填充 (数据有效或超时)
// 返回 false → 数据尚未到达，本次 loop 跳过
// ─────────────────────────────────────────────
bool ModbusController::pollReadResult(uint16_t &outValue) {
    if (!_rxPending) return false;

    while (Serial1.available() && _rxCount < 7) {
        _rxBuf[_rxCount++] = Serial1.read();
    }

    bool completed = false;

    if (_rxCount >= 7) {
        completed = true;
        if (_rxBuf[0] != MODBUS_SERVO_SLAVE_ID || _rxBuf[1] != 0x03) {
            outValue = 0xFFFE;
        } else {
            outValue = ((uint16_t)_rxBuf[3] << 8) | _rxBuf[4];
        }
    } else if (millis() - _rxStartMs > RX_TIMEOUT_MS) {
        completed = true;
        outValue = 0xFFFF; // 超时
    }

    if (completed) {
        _rxPending = false;

        // ── 总线仲裁: 立即执行被缓冲的写操作 ──
        if (_pendingWriteValid) {
            _pendingWriteValid = false;
            _doWrite(_pendingWriteAddr, _pendingWriteValue);
        }
    }

    return completed;
}

// ─────────────────────────────────────────────
// 废弃阻塞读 (仅低频配置场景)
// ─────────────────────────────────────────────
uint16_t ModbusController::readRegisterSync(uint16_t regAddr) {
    requestRegisterRead(regAddr);
    uint32_t start = millis();
    while (millis() - start < RX_TIMEOUT_MS * 2) {
        uint16_t val;
        if (pollReadResult(val)) return val;
    }
    return 0xFFFF;
}
