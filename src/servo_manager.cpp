#include "servo_manager.h"
#include "config.h"

void ServoManager::begin() {
    ModbusController::getInstance()->initialize();
    _state = SYS_IDLE;
    _stateTimer = millis();
    _targetMode = 1; // 默认速度模式，可由外部 setTargetMode 修改
    _targetCommand = 0;
    _lastSentCommand = -9999;
    _lastMonitorMs = 0;
}

void ServoManager::update() {
    uint32_t elapsed = millis() - _stateTimer;

    // 1. 周期监控 (10-20Hz 轮询即可)
    if (millis() - _lastMonitorMs > 100) {
        updateMonitor();
        _lastMonitorMs = millis();
    }

    // 2. 工业标准状态机核心逻辑
    switch (_state) {
        case SYS_IDLE:
            // 等待系统启动或指令
            _state = SYS_INIT;
            _stateTimer = millis();
            break;

        case SYS_INIT:
            // 初始化阶段：确认链路连通性
            if (_data.statusWord != 0xFFFF) {
                Serial.println("[Servo] Link OK. Checking Alarms...");
                _state = SYS_CHECK_ALARM;
                _stateTimer = millis();
            } else if (elapsed > 5000) {
                // 通讯超时，保持初始化态重试
                _stateTimer = millis();
            }
            break;

        case SYS_CHECK_ALARM:
            // 报警自检
            if (_data.alarmCode == 0) {
                _state = SYS_SERVO_ENABLE;
                _stateTimer = millis();
            } else {
                Serial.printf("[Servo] Alarm Detected: ALM-%02d. Resetting...\n", _data.alarmCode);
                _state = SYS_RESET_ALARM;
                _stateTimer = millis();
            }
            break;

        case SYS_RESET_ALARM:
            // 复位报警 (PA60=2 或特定复位指令)
            if (elapsed < 500) {
                ModbusController::getInstance()->writeRegister(0x2000, 2); // 通用复位指令
            } else {
                _state = SYS_CHECK_ALARM;
                _stateTimer = millis();
            }
            break;

        case SYS_SERVO_ENABLE:
            // 伺服使能 (SON)
            // 先确保 RAM 中的使能位打开
            ModbusController::getInstance()->writeRegister(MODBUS_SERVO_ENABLE_REG, 1);
            if (_data.statusWord & 0x01) { // 假设 StatusWord bit 0 是 Servo ON 状态
                Serial.println("[Servo] Enabled. Setting Parameters...");
                _state = SYS_SET_TORQUE;
                _stateTimer = millis();
            } else if (elapsed > 2000) {
                _state = SYS_FAULT; // 使能超时进入故障态
            }
            break;

        case SYS_SET_TORQUE:
            // 参数设定：限速 + 模式确认
            // 普菲德/其它伺服通常在扭矩模式下需要先写限速寄存器
            // 写入限速 (PA31: 扭矩模式指令速度限制)
            ModbusController::getInstance()->writeRegister(MODBUS_SERVO_TRQ_LIMIT_SPD_REG, 500); // 默认 500 RPM 安全限速
            
            // 检查当前物理模式是否匹配，如果不匹配可能需要 L1 层的存盘重启逻辑 (已在之前 L1 处理)
            // 这里假设模式已由外部或初始化配合好
            _state = SYS_RUNNING;
            _stateTimer = millis();
            break;

        case SYS_RUNNING:
            // 正常运行：实时调节扭矩/速度
            if (_data.alarmCode != 0) {
                _state = SYS_FAULT;
                break;
            }

            // 根据 targetMode 下发指令 (RAM 寄存器)
            if (abs(_targetCommand - _lastSentCommand) >= 2) {
                if (_targetMode == 1) { // 速度模式
                    ModbusController::getInstance()->writeRegister(MODBUS_SERVO_SPEED_REG, (uint16_t)abs(_targetCommand));
                } else if (_targetMode == 2) { // 扭矩模式
                    ModbusController::getInstance()->writeRegister(MODBUS_SERVO_FWD_TRQ_REG, (uint16_t)abs(_targetCommand));
                    ModbusController::getInstance()->writeRegister(MODBUS_SERVO_REV_TRQ_REG, (uint16_t)abs(_targetCommand));
                }
                _lastSentCommand = _targetCommand;
            }
            break;

        case SYS_STOP:
            // 受控停止
            ModbusController::getInstance()->writeRegister(MODBUS_SERVO_SPEED_REG, 0);
            ModbusController::getInstance()->writeRegister(MODBUS_SERVO_FWD_TRQ_REG, 0);
            ModbusController::getInstance()->writeRegister(MODBUS_SERVO_REV_TRQ_REG, 0);
            ModbusController::getInstance()->writeRegister(MODBUS_SERVO_ENABLE_REG, 0); // 去使能
            _state = SYS_IDLE;
            _stateTimer = millis();
            break;

        case SYS_FAULT:
            // 故障锁定：只能通过人工 setTargetMode(SYS_IDLE) 或 reset 退出
            break;
    }
}

void ServoManager::updateMonitor() {
    // 工业方案通常一次读取多个寄存器，这里简化为连续同步读取
    _data.statusWord = ModbusController::getInstance()->readRegisterSync(0x2001);
    
    // 只有在通讯正常时才读取其它数据
    if (_data.statusWord != 0xFFFF) {
        _data.alarmCode = ModbusController::getInstance()->readRegisterSync(0x1013);
        _data.currentMode = (int)ModbusController::getInstance()->readRegisterSync(0x1009); // 物理当前模式
        _data.actualSpeed = (int16_t)ModbusController::getInstance()->readRegisterSync(0x1000);
        _data.actualTorque = (float)((int16_t)ModbusController::getInstance()->readRegisterSync(0x1007));
    } else {
        // 通讯丢失
        if (_state != SYS_INIT && _state != SYS_IDLE) {
            _state = SYS_INIT; // 自动重连进入初始化
            _stateTimer = millis();
        }
    }
}

const char* ServoManager::getStateName() const {
    switch (_state) {
        case SYS_IDLE:         return "IDLE";
        case SYS_INIT:         return "INIT";
        case SYS_CHECK_ALARM:  return "ALARM_CHK";
        case SYS_RESET_ALARM:  return "ALARM_RST";
        case SYS_SERVO_ENABLE: return "ENABLING";
        case SYS_SET_TORQUE:   return "PARAM_SET";
        case SYS_RUNNING:      return "RUNNING";
        case SYS_STOP:         return "STOPPING";
        case SYS_FAULT:        return "FAULT";
        default:               return "?STATE?";
    }
}

void ServoManager::setTargetMode(int mode) {
    if (_targetMode != mode) {
        _targetMode = mode;
        // 如果机器正在运行，更改模式应触发 STOP -> INIT 重新配置流程
        if (_state == SYS_RUNNING) {
            _state = SYS_STOP;
        }
    }
}

void ServoManager::setTargetCommand(int value) {
    _targetCommand = value;
}

void ServoManager::resetAlarm() {
    if (_state == SYS_FAULT || _state == SYS_CHECK_ALARM) {
        _state = SYS_RESET_ALARM;
        _stateTimer = millis();
    }
}
