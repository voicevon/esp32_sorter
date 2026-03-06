#ifndef MODBUS_CONTROLLER_H
#define MODBUS_CONTROLLER_H

#include <Arduino.h>

/**
 * @brief ModbusController 类
 * 采用单例模式封装 PS100 伺服驱动器的 Modbus RTU 通讯逻辑
 */
class ModbusController {
public:
    static ModbusController* getInstance() {
        static ModbusController instance;
        return &instance;
    }

    // 初始化串口与引脚
    void initialize();

    // 核心控制接口
    void setSpeed(int speedRpm);
    void softReset();
    void setEnable(bool enable);
    
    // 状态同步
    void syncParameters(bool isSpeedMode, bool force = false);

    // 数据读取
    void requestRegisterRead(uint16_t regAddr);

    // 状态查询
    int getLastSentSpeed() const { return _lastSentSpeed; }
    int getLastSyncedMode() const { return _lastSyncedServoMode; }

private:
    ModbusController() : _lastSentSpeed(-1), _lastSyncedServoMode(-1) {}
    
    // 禁止拷贝
    ModbusController(const ModbusController&) = delete;
    ModbusController& operator=(const ModbusController&) = delete;

    // 底层通讯
    void writeRegister(uint16_t regAddr, uint16_t regValue);
    uint16_t calculateCRC16(const uint8_t *data, uint16_t length);

    // 内部变量
    int _lastSentSpeed;
    int _lastSyncedServoMode; // -1: Unknown, 0: Positions, 1: Speed
};

#endif // MODBUS_CONTROLLER_H
