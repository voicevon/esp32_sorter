/**
 * @file ModbusMaster.cpp
 * @brief 轻量级 Modbus RTU 主机驱动实现
 *
 * 移植自 esp32_weight_master/src/drivers/ModbusMaster.cpp
 * 主要变更：
 *   Serial1 → Serial2 (UART2，RX:GPIO16, TX:GPIO17)
 *   LogLevel 枚举 → ModbusLogLevel（内联，避免与其他模块冲突）
 *   移除 PinDefinition.h / SystemTypes.h 依赖
 */
#include "ModbusMaster.h"
#include <Arduino.h>

ModbusMaster* ModbusMaster::_instance = nullptr;

ModbusMaster::ModbusMaster(int rxPin, int txPin, int enPin, long baud)
    : _rxPin(rxPin), _txPin(txPin), _enPin(enPin), _baud(baud) {
    _instance = this;
    _mutexBus = xSemaphoreCreateMutex();
}

void ModbusMaster::begin() {
    // 使用 Serial2 (UART2) — 与 Serial (debug) 及 Serial1 互不干扰
    Serial2.begin(_baud, SERIAL_8N2, _rxPin, _txPin);
    _charTimeUs = 1000000UL * 11 / _baud; // 单个字符(11位)传输所需微秒数
    if (_enPin >= 0) {
        pinMode(_enPin, OUTPUT);
        digitalWrite(_enPin, LOW); // 默认接收模式
    }
    // 启动专用接收任务，绑定 Core 1，优先级 15（高于 UITask）
    xTaskCreatePinnedToCore(modbusTask, "HMI_MB", 4096, this, 15, &_taskHandle, 1);
    if (_logLevel >= MBUS_LOG_INFO) {
        Serial.printf("[ModbusMaster] begin() RX:%d TX:%d EN:%d Baud:%ld\n",
                      _rxPin, _txPin, _enPin, _baud);
    }
}

// ── CRC16 查表 ────────────────────────────────────────────────────────────────
static const uint16_t crcTable[] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

uint16_t ModbusMaster::calculateCRC(uint8_t* buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crcTable[(crc ^ buf[i]) & 0xFF];
    }
    return crc;
}

void ModbusMaster::sendPacket(uint8_t* buf, int len) {
    uint16_t crc = calculateCRC(buf, len);
    buf[len++] = crc & 0xFF;
    buf[len++] = (crc >> 8) & 0xFF;

    if (_logLevel >= MBUS_LOG_VERBOSE) {
        Serial.print("[MB TX>] ");
        for (int i = 0; i < len; i++) Serial.printf("%02X ", buf[i]);
        Serial.println();
    }

    if (_enPin >= 0) {
        digitalWrite(_enPin, HIGH);
        delayMicroseconds(_charTimeUs / 2); // 等待电平稳定
    }
    Serial2.write(buf, len);
    Serial2.flush();                       // 等待 UART FIFO 发空
    delayMicroseconds(_charTimeUs);        // 等待移位寄存器发完最后一位
    if (_enPin >= 0) {
        digitalWrite(_enPin, LOW);         // 切回接收模式
    }
}

// ── 异步读 (FC 0x03) ──────────────────────────────────────────────────────────
bool ModbusMaster::asyncRead(uint8_t id, uint16_t addr, uint16_t count,
                              cbTransaction cb, uint16_t* destBuffer) {
    if (xSemaphoreTake(_mutexBus, pdMS_TO_TICKS(10)) != pdTRUE) return false;
    if (_status == ST_WAITING) {
        xSemaphoreGive(_mutexBus);
        return false; // 总线忙
    }

    _lastTid     = id;
    _pendingCb   = cb;
    _pendingData = (void*)destBuffer;
    _status      = ST_WAITING;
    _lastPollTime = millis();
    _packetsSent++;

    _txBuf[0] = id;
    _txBuf[1] = 0x03;
    _txBuf[2] = addr >> 8;
    _txBuf[3] = addr & 0xFF;
    _txBuf[4] = count >> 8;
    _txBuf[5] = count & 0xFF;

    while (Serial2.available()) Serial2.read(); // 清空残留数据
    vTaskDelay(pdMS_TO_TICKS(5));               // 强制总线空闲间隔
    sendPacket(_txBuf, 6);

    if (_logLevel >= MBUS_LOG_VERBOSE) {
        Serial.printf("[ModbusMaster] asyncRead  ID:%d Addr:0x%04X Cnt:%d\n", id, addr, count);
    }
    xSemaphoreGive(_mutexBus);
    return true;
}

// ── 异步写 (FC 0x06) ──────────────────────────────────────────────────────────
bool ModbusMaster::asyncWrite(uint8_t id, uint16_t addr, uint16_t value,
                               cbTransaction cb) {
    if (xSemaphoreTake(_mutexBus, pdMS_TO_TICKS(10)) != pdTRUE) return false;
    if (_status == ST_WAITING) {
        xSemaphoreGive(_mutexBus);
        return false;
    }

    _lastTid     = id;
    _pendingCb   = cb;
    _pendingData = nullptr;
    _status      = ST_WAITING;
    _lastPollTime = millis();
    _packetsSent++;

    _txBuf[0] = id;
    _txBuf[1] = 0x06;
    _txBuf[2] = addr >> 8;
    _txBuf[3] = addr & 0xFF;
    _txBuf[4] = value >> 8;
    _txBuf[5] = value & 0xFF;

    while (Serial2.available()) Serial2.read();
    vTaskDelay(pdMS_TO_TICKS(5));
    sendPacket(_txBuf, 6);

    if (_logLevel >= MBUS_LOG_VERBOSE) {
        Serial.printf("[ModbusMaster] asyncWrite ID:%d Addr:0x%04X Val:%d\n", id, addr, value);
    }
    xSemaphoreGive(_mutexBus);
    return true;
}

// ── 同步写 (FC 0x06, 阻塞) ───────────────────────────────────────────────────
bool ModbusMaster::syncWrite(uint8_t id, uint16_t addr, uint16_t value) {
    if (xSemaphoreTake(_mutexBus, pdMS_TO_TICKS(500)) != pdTRUE) return false;

    _status       = ST_WAITING;
    _lastPollTime = millis();
    _txBuf[0] = id;
    _txBuf[1] = 0x06;
    _txBuf[2] = addr >> 8;
    _txBuf[3] = addr & 0xFF;
    _txBuf[4] = value >> 8;
    _txBuf[5] = value & 0xFF;

    while (Serial2.available()) Serial2.read();
    sendPacket(_txBuf, 6);
    xSemaphoreGive(_mutexBus);

    unsigned long start = millis();
    while (millis() - start < 1000) {
        if (_status == ST_SUCCESS) return true;
        if (_status == ST_ERROR || _status == ST_TIMEOUT) return false;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}

// ── 广播写 (ID=0, 无响应) ─────────────────────────────────────────────────────
bool ModbusMaster::broadcastWrite(uint16_t addr, uint16_t value) {
    if (xSemaphoreTake(_mutexBus, pdMS_TO_TICKS(500)) != pdTRUE) return false;
    _txBuf[0] = 0x00; // 广播 ID
    _txBuf[1] = 0x06;
    _txBuf[2] = addr >> 8;
    _txBuf[3] = addr & 0xFF;
    _txBuf[4] = value >> 8;
    _txBuf[5] = value & 0xFF;
    sendPacket(_txBuf, 6);
    xSemaphoreGive(_mutexBus);
    return true;
}

// ── 内部接收任务 ──────────────────────────────────────────────────────────────
void ModbusMaster::modbusTask(void* param) {
    ModbusMaster* self = (ModbusMaster*)param;

    while (true) {
        if (self->_status == ST_WAITING) {
            if (Serial2.available()) {
                // 收集完整帧：等到总线静默 (3.5 字符间隔 ≈ 15ms)
                unsigned long lastByteTime = millis();
                int idx = 0;
                while (millis() - lastByteTime < 15) {
                    if (Serial2.available()) {
                        self->_rxBuf[idx++] = Serial2.read();
                        lastByteTime = millis();
                        if (idx >= 256) break;
                    }
                    vTaskDelay(1);
                }

                if (idx > 0 && self->_logLevel >= MBUS_LOG_VERBOSE) {
                    Serial.print("[MB RX<] ");
                    for (int i = 0; i < idx; i++) Serial.printf("%02X ", self->_rxBuf[i]);
                    Serial.println();
                }

                if (idx >= 5) { // 最小帧: ID + FN + DATA(1) + CRC(2)
                    uint16_t calcCrc = self->calculateCRC(self->_rxBuf, idx - 2);
                    uint16_t rxCrc   = self->_rxBuf[idx-2] | (self->_rxBuf[idx-1] << 8);

                    if (calcCrc == rxCrc) {
                        uint8_t fn = self->_rxBuf[1];

                        if (fn == 0x03) {
                            // 读回复：将寄存器值存入目标缓冲区
                            int byteCount = self->_rxBuf[2];
                            uint16_t* dest = (uint16_t*)self->_pendingData;
                            if (dest) {
                                for (int i = 0; i < byteCount / 2; i++) {
                                    dest[i] = ((uint16_t)self->_rxBuf[3 + i * 2] << 8)
                                               | self->_rxBuf[4 + i * 2];
                                }
                            }
                            if (self->_pendingCb) {
                                cbTransaction cb = self->_pendingCb;
                                self->_pendingCb = nullptr;
                                cb(Modbus::EX_SUCCESS, self->_lastTid, self->_pendingData);
                            }
                            self->_status = ST_SUCCESS;

                        } else if (fn == 0x06) {
                            // 写回显：正常 Echo
                            if (self->_pendingCb) {
                                cbTransaction cb = self->_pendingCb;
                                self->_pendingCb = nullptr;
                                cb(Modbus::EX_SUCCESS, self->_lastTid, nullptr);
                            }
                            self->_status = ST_SUCCESS;

                        } else if ((fn & 0x80) != 0) {
                            // 从机异常回复 (0x83, 0x86, ...)
                            if (self->_logLevel >= MBUS_LOG_ERROR) {
                                Serial.printf("[ModbusMaster] EXCEPTION ID:%d FN:0x%02X Code:0x%02X\n",
                                              self->_rxBuf[0], fn, self->_rxBuf[2]);
                            }
                            if (self->_pendingCb) {
                                cbTransaction cb = self->_pendingCb;
                                self->_pendingCb = nullptr;
                                cb(Modbus::EX_ERROR, self->_lastTid, nullptr);
                            }
                            self->_status = ST_ERROR;

                        } else {
                            // 收到非预期的功能码 (例如 HMI 作为 Master 误发出的 0x02 轮询请求)
                            // 在多主机或 HMI 配置不当时常见。此处仅记录并忽略，不终止当前等待的事务，
                            // 这样真正的从机回复（如 0x03/0x06）仍有机会在超时前被接收。
                            if (self->_logLevel >= MBUS_LOG_INFO) {
                                Serial.printf("[ModbusMaster] IGNORE stray frame: FN:0x%02X ID:%d. Raw: ",
                                              fn, self->_rxBuf[0]);
                                for (int i = 0; i < idx; i++) Serial.printf("%02X ", self->_rxBuf[i]);
                                Serial.println("(Likely HMI auto-poll, check HMI config)");
                            }
                            // 不清除 _status 和 _pendingCb，让循环继续等待
                        }

                    } else {
                        if (self->_logLevel >= MBUS_LOG_ERROR) {
                            Serial.printf("[ModbusMaster] CRC ERR ID:%d Calc:%04X Rx:%04X\n",
                                          self->_rxBuf[0], calcCrc, rxCrc);
                        }
                        if (self->_pendingCb) {
                            cbTransaction cb = self->_pendingCb;
                            self->_pendingCb = nullptr;
                            cb(Modbus::EX_ERROR, self->_lastTid, nullptr);
                        }
                        self->_status = ST_ERROR;
                    }
                }
                // 帧长 < 5 字节：忽略（可能是噪声），继续等待

            } else if (millis() - self->_lastPollTime > 2000) {
                // 超时（2s 内无响应）
                if (self->_logLevel >= MBUS_LOG_ERROR) {
                    Serial.printf("[ModbusMaster] TIMEOUT ID:%d\n", self->_lastTid);
                }
                if (self->_pendingCb) {
                    cbTransaction cb = self->_pendingCb;
                    self->_pendingCb = nullptr;
                    cb(Modbus::EX_TIMEOUT, self->_lastTid, self->_pendingData);
                }
                self->_status = ST_TIMEOUT;
                self->_packetsDropped++;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5)); // 5ms 轮询间隔，不饿死其他任务
    }
}

// ── 原始字节诊断接口 ──────────────────────────────────────────────────────────
void ModbusMaster::sendRawBuffer(const uint8_t* buf, int len) {
    if (xSemaphoreTake(_mutexBus, pdMS_TO_TICKS(100)) != pdTRUE) return;
    if (_enPin >= 0) digitalWrite(_enPin, HIGH);
    Serial2.write(buf, len);
    Serial2.flush();
    if (_enPin >= 0) digitalWrite(_enPin, LOW);
    xSemaphoreGive(_mutexBus);
}

void ModbusMaster::sendRawByte(uint8_t byte) {
    if (_enPin >= 0) digitalWrite(_enPin, HIGH);
    Serial2.write(byte);
    Serial2.flush();
    if (_enPin >= 0) digitalWrite(_enPin, LOW);
}

int     ModbusMaster::availableRaw() { return Serial2.available(); }
uint8_t ModbusMaster::readRawByte()  { return Serial2.read(); }
void    ModbusMaster::clearRawBuffer() { while (Serial2.available()) Serial2.read(); }
