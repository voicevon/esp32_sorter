#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include <Arduino.h>
#include <functional>

/**
 * @file ModbusMaster.h
 * @brief 轻量级 Modbus RTU 主机驱动 (移植自 esp32_weight_master)
 * @location src/utils/ModbusMaster.h
 *
 * 特性：
 *  - 自研协议组包 + CRC16 查表校验，零第三方库依赖
 *  - 异步/同步双接口：asyncRead/asyncWrite 用于非阻塞轮询，syncWrite 用于初始化
 *  - 专用 FreeRTOS 任务接收从机响应，不占用调用方任务
 *  - 支持 RS-485 方向控制引脚 (DE/RE)
 *  - 使用 Serial2 (UART2)，不影响 Serial (调试) 和 Serial1 (其他用途)
 *
 * 适配说明 (vs esp32_weight_master)：
 *  - 移除了对 PinDefinition.h 和 SystemTypes.h 的依赖
 *  - LogLevel 定义内联进本文件
 *  - Serial1 → Serial2
 *  - 单例静态指针保留（本项目只创建一个 ModbusMaster 实例）
 */

// ── 日志级别 ─────────────────────────────────────────────────────────────────
enum ModbusLogLevel {
    MBUS_LOG_NONE    = 0,
    MBUS_LOG_ERROR   = 1,
    MBUS_LOG_INFO    = 2,
    MBUS_LOG_VERBOSE = 3
};

// ── Modbus 结果枚举 ───────────────────────────────────────────────────────────
namespace Modbus {
    enum ResultCode {
        EX_SUCCESS = 0x00,
        EX_TIMEOUT = 0xE4,
        EX_ERROR   = 0xFF
    };
}

typedef std::function<bool(Modbus::ResultCode, uint16_t, void*)> cbTransaction;

/**
 * @class ModbusMaster
 * @brief Modbus RTU 主机驱动（异步 + 同步双接口）
 *
 * 使用方法：
 *   ModbusMaster modbus(RX_PIN, TX_PIN, EN_PIN, 115200);
 *   modbus.begin();            // 在 setup() 中调用，启动内部 FreeRTOS 任务
 *   modbus.asyncRead(...);     // 在 vUITask 中轮询读取
 *   modbus.asyncWrite(...);    // 在需要时非阻塞写入
 *   modbus.syncWrite(...);     // 在 setup() 中同步写入（初始化配置）
 */
class ModbusMaster {
public:
    enum TransactionStatus {
        ST_IDLE,      // 空闲
        ST_WAITING,   // 等待从机 ACK
        ST_SUCCESS,   // 上次事务成功
        ST_TIMEOUT,   // 上次事务超时
        ST_ERROR      // 上次事务出错
    };

    /**
     * @param rxPin   UART2 RX 引脚
     * @param txPin   UART2 TX 引脚
     * @param enPin   RS-485 DE/RE 方向控制引脚（-1 表示无需控制）
     * @param baud    波特率
     */
    ModbusMaster(int rxPin, int txPin, int enPin, long baud);

    /** 初始化 UART2 并启动内部接收任务，在 setup() 中调用 */
    void begin();

    TransactionStatus getStatus() const { return _status; }

    /**
     * @brief 异步读取多个保持寄存器 (FC 0x03)
     * @param id          从机地址
     * @param addr        起始寄存器偏移 (0-based)
     * @param count       读取寄存器数量
     * @param cb          完成回调 (可为 nullptr)
     * @param destBuffer  存储读回值的缓冲区（调用方负责生命周期）
     */
    bool asyncRead(uint8_t id, uint16_t addr, uint16_t count, cbTransaction cb, uint16_t* destBuffer);

    /**
     * @brief 异步写入单个保持寄存器 (FC 0x06)
     * @param id    从机地址
     * @param addr  寄存器偏移 (0-based)
     * @param value 写入值
     * @param cb    完成回调 (可为 nullptr)
     */
    bool asyncWrite(uint8_t id, uint16_t addr, uint16_t value, cbTransaction cb);

    /**
     * @brief 同步写入单个保持寄存器 (FC 0x06)
     * 阻塞直到收到回显或超时（最长 1s），适用于 setup() 中的初始化写入。
     */
    bool syncWrite(uint8_t id, uint16_t addr, uint16_t value);

    /**
     * @brief 广播写入 (ID=0，从机不回复)
     * 用于同步广播类指令，不等待响应。
     */
    bool broadcastWrite(uint16_t addr, uint16_t value);

    void setLogLevel(ModbusLogLevel level) { _logLevel = level; }
    ModbusLogLevel getLogLevel() const { return _logLevel; }

    // 原始字节诊断接口（调试用）
    void    sendRawBuffer(const uint8_t* buf, int len);
    void    sendRawByte(uint8_t byte);
    int     availableRaw();
    uint8_t readRawByte();
    void    clearRawBuffer();

    uint32_t getPacketsSent()   const { return _packetsSent; }
    uint32_t getPacketsDropped() const { return _packetsDropped; }

private:
    int  _rxPin, _txPin, _enPin;
    long _baud;

    volatile TransactionStatus _status   = ST_IDLE;
    unsigned long              _lastPollTime = 0;

    cbTransaction     _pendingCb   = nullptr;
    void*             _pendingData = nullptr;
    volatile uint16_t _lastTid     = 0;

    uint8_t  _txBuf[32];
    uint8_t  _rxBuf[256];
    int      _rxLen = 0;

    uint32_t _packetsSent    = 0;
    uint32_t _packetsDropped = 0;
    uint32_t _charTimeUs     = 0;

    ModbusLogLevel _logLevel = MBUS_LOG_INFO;

    SemaphoreHandle_t _mutexBus;
    TaskHandle_t      _taskHandle = nullptr;

    uint16_t calculateCRC(uint8_t* buf, int len);
    void     sendPacket(uint8_t* buf, int len);
    bool     validateResponse(uint8_t* buf, int len, uint8_t expectedId, uint8_t expectedFn);

    static ModbusMaster* _instance;
    static void modbusTask(void* param);
};

#endif // MODBUS_MASTER_H
