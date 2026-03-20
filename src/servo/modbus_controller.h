#ifndef MODBUS_CONTROLLER_H
#define MODBUS_CONTROLLER_H

#include <Arduino.h>

/**
 * @brief ModbusController — 非阻塞 Modbus RTU 驱动 (带总线仲裁)
 *
 * 读写互斥保证:
 *   - 若当前有读请求正在飞行 (_rxPending=true)，writeRegister() 将写操作缓冲
 *     至单槽写队列 (_pendingWrite)，在 pollReadResult() 完成后立即执行
 *   - 这保证了 Modbus 总线在任意时刻只有一个事务在进行
 *
 * 异步读取流程:
 *   1. requestRegisterRead(reg) → 发出读帧，立即返回
 *   2. pollReadResult(val)      → 在后续 loop() 中检查，无阻塞
 *      返回 true  → outValue 有效  |  返回 false → 数据未到
 */
class ModbusController {
public:
    static ModbusController* getInstance() {
        static ModbusController instance;
        return &instance;
    }

    void initialize();

    // === 写寄存器 (带仲裁: 若读在飞行中则自动延迟至读完成后执行) ===
    void writeRegister(uint16_t regAddr, uint16_t regValue);

    // === 异步读取 ===
    void requestRegisterRead(uint16_t regAddr);
    bool pollReadResult(uint16_t &outValue);   // non-blocking

    // === 废弃阻塞读 (仅低频配置场景) ===
    uint16_t readRegisterSync(uint16_t regAddr);

private:
    ModbusController()
        : _rxCount(0), _rxPending(false), _rxStartMs(0),
          _pendingWriteValid(false), _pendingWriteAddr(0), _pendingWriteValue(0) {}
    ModbusController(const ModbusController&) = delete;
    ModbusController& operator=(const ModbusController&) = delete;

    // 底层发送 (内部使用)
    void _doWrite(uint16_t regAddr, uint16_t regValue);
    uint16_t calculateCRC16(const uint8_t *data, uint16_t length);

    // 异步接收缓冲区
    uint8_t  _rxBuf[7];
    int      _rxCount;
    bool     _rxPending;
    uint32_t _rxStartMs;
    static const uint32_t RX_TIMEOUT_MS = 50;

    // 单槽写队列 (总线仲裁缓冲)
    bool     _pendingWriteValid;
    uint16_t _pendingWriteAddr;
    uint16_t _pendingWriteValue;
};

extern unsigned long lastModbusSendTime;

#endif // MODBUS_CONTROLLER_H
