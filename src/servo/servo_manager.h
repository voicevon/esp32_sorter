#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <Arduino.h>
#include "modbus_controller.h"

// 工业标准伺服状态机 (Servo State Machine - Industrial Standard)
enum ServoState {
    SYS_IDLE,            // 空闲：等待启动指令
    SYS_INIT,            // 初始化：检测 Modbus 通讯、波特率匹配
    SYS_CHECK_ALARM,      // 报警自检：读取伺服驱动器当前报警代码
    SYS_RESET_ALARM,     // 报警复位：尝试清除驱动器当前故障 (故障确认)
    SYS_SERVO_ENABLE,    // 伺服使能：下发 SON 指令使电机进入持有态
    SYS_SET_TORQUE,      // 参数设定：写入限速值与目标扭矩/速度 (防飞车关键)
    SYS_RUNNING,         // 正常运行：实时监控负载与指令更新
    SYS_STOP,            // 受控停止：指令归零并安全去使能
    SYS_FAULT            // 故障锁定：发生无法自愈的错误，需人工干预或重启
};

struct ServoMonitorData {
    int      actualSpeed;    // 0x1000
    float    actualTorque;   // 0x1007
    long     position;
    uint16_t statusWord;     // 0x2001
    uint16_t alarmCode;      // 0x1013
    int      currentMode;    // 0x1009
    uint16_t phaseCurrent;   // 0x1008  (0.1A/bit)
    uint16_t busVoltage;     // 0x1012  (V)
};

class ServoManager {
public:
    static ServoManager& getInstance() {
        static ServoManager instance;
        return instance;
    }

    void begin();
    void update(); // Main Loop Hook

    // 向系统宣告控制意图
    void setTargetMode(int mode);      // 切换目标模式 (1:速, 2:扭, 0:位)
    void setTargetCommand(int value);  // 设置目标给定值
    void resetAlarm();

    // 状态查询
    const char* getStateName() const;
    const ServoMonitorData& getData() const { return _data; }
    ServoState getState() const { return _state; }

private:
    ServoManager() : _state(SYS_IDLE), _targetMode(1), _targetCommand(0) {}
    
    // 标准状态机持有状态
    ServoState _state;
    ServoMonitorData _data;

    int _targetMode;
    int _targetCommand;
    int _lastSentCommand;
    uint32_t _lastMonitorMs;
    uint32_t _stateTimer;

    void updateMonitor();      // 非阻塞异步采集

    // 异步监控状态
    uint8_t  _monitorStep;    // 当前轮询的寄存器序号 (0-4)
    bool     _monitorPending; // 是否有一个正在等待 pollReadResult 的请求
};

#endif
