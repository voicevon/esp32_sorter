#ifndef MODBUS_CONTROLLER_H
#define MODBUS_CONTROLLER_H

#include <Arduino.h>

/**
 * @brief ModbusController - 非阻塞 Modbus RTU 驱动
 *
 * 读取模式 (异步):
 *   1. 调用 requestRegisterRead(reg) 发出读请求，立即返回
 *   2. 在后续 loop() 中调用 pollReadResult(value): 
 *      - 返回 true  → value 已填充，可使用
 *      - 返回 false → 响应尚未就绪，本次 loop 跳过即可
 *
 * 写入模式 (同步): writeRegister() 仅占用约 1ms (串口发送时间)，无阻塞等待。
 */
class ModbusController {
public:
    static ModbusController* getInstance() {
        static ModbusController instance;
        return &instance;
    }

    void initialize();

    // === 写入 (同步，约 1ms，可安全调用) ===
    void writeRegister(uint16_t regAddr, uint16_t regValue);

    // === 读取 (异步，非阻塞) ===
    // Step 1: 发出请求（立即返回）
    void requestRegisterRead(uint16_t regAddr);
    // Step 2: 在下一次 loop() 中轮询响应
    // 返回 true → result 已写入 outValue; 返回 false → 数据未到，继续等
    bool pollReadResult(uint16_t &outValue);

    // === 已废弃，仅保留给 ServoConfigHandler 的配置写入等不频繁场景 ===
    // 不可在主循环高频路径调用
    uint16_t readRegisterSync(uint16_t regAddr);

private:
    ModbusController() : _rxCount(0), _rxPending(false), _rxStartMs(0) {}
    ModbusController(const ModbusController&) = delete;
    ModbusController& operator=(const ModbusController&) = delete;

    uint16_t calculateCRC16(const uint8_t *data, uint16_t length);

    // 异步接收缓冲区
    uint8_t  _rxBuf[7];
    int      _rxCount;
    bool     _rxPending;       // 是否有一个正在等待的读请求
    uint32_t _rxStartMs;       // 请求发出的时间戳，用于超时判断
    static const uint32_t RX_TIMEOUT_MS = 50; // 超时从 150ms 缩短至 50ms
};

extern unsigned long lastModbusSendTime;

#endif // MODBUS_CONTROLLER_H
