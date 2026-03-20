#ifndef SERVO_MONITOR_HANDLER_H
#define SERVO_MONITOR_HANDLER_H

#include "base_diagnostic_handler.h"
#include "../servo/servo_manager.h"
#include "../user_interface/user_interface.h"

/**
 * @brief ServoMonitorHandler — 伺服实时状态仪表盘
 *
 * 完全从 ServoManager::getData() 缓存读取数据，
 * 不发起任何直接 Modbus 通讯，无阻塞。
 *
 * 数据刷新由 ServoManager::update() 的异步轮询驱动，
 * 7 个寄存器按序轮转，每轮约 7×(50ms) = 350ms 完成一次完整采样。
 */
class ServoMonitorHandler : public BaseDiagnosticHandler {
private:
    UserInterface* userInterface;
    uint32_t lastRefreshMs = 0;
    int currentPage = 0; // 0: 概览 | 1: 实时数据 | 2: 电气

public:
    ServoMonitorHandler(UserInterface* ui) : userInterface(ui) {}

    void begin() override {
        Serial.println("[Dashboard] Servo Monitor Started");
        lastRefreshMs = 0;
        currentPage = 0;
    }

    void update(uint32_t currentMs, bool btnPressed) override {
        if (btnPressed) {
            handleReturnToMenu();
            return;
        }

        int delta = userInterface->getEncoderDelta();
        if (delta != 0) {
            currentPage = (currentPage + delta + 3) % 3;
            lastRefreshMs = 0; // 立即刷新
        }

        if (currentMs - lastRefreshMs >= 300) {
            lastRefreshMs = currentMs;
            refreshDashboard();
        }
    }

    void end() override {
        Serial.println("[Dashboard] Servo Monitor Stopped");
    }

private:
    void refreshDashboard() {
        const auto& d  = ServoManager::getInstance().getData();
        const auto& sm = ServoManager::getInstance();

        if (currentPage == 0) {
            // PAGE 0: 概览
            String modeStr  = "Unknown";
            if (d.currentMode == 0) modeStr = "Position";
            else if (d.currentMode == 1) modeStr = "Speed";
            else if (d.currentMode == 2) modeStr = "Torque";

            String stateStr = sm.getStateName();
            String alarmStr = (d.alarmCode == 0xFFFF) ? "ERR" :
                              (d.alarmCode == 0)      ? "Normal" :
                              "ALM-" + String(d.alarmCode);

            userInterface->displayDiagnosticInfo("Servo Overview [1/3]",
                "State:  " + stateStr  + "\n" +
                "Mode:   " + modeStr   + "\n" +
                "Alarm:  " + alarmStr  + "\n" +
                "  < Knob to Flip >");
        }
        else if (currentPage == 1) {
            // PAGE 1: 实时数据
            String speedStr   = (d.statusWord == 0xFFFF) ? "ERR" :
                                String(d.actualSpeed) + " RPM";
            String torqueStr  = (d.statusWord == 0xFFFF) ? "ERR" :
                                String(d.actualTorque, 1) + " %";
            String currStr    = (d.phaseCurrent == 0xFFFF) ? "ERR" :
                                String(d.phaseCurrent / 10.0f, 1) + " A";

            userInterface->displayDiagnosticInfo("Live Data [2/3]",
                "Speed:  " + speedStr  + "\n" +
                "Torque: " + torqueStr + "\n" +
                "Curr:   " + currStr   + "\n" +
                "  < Knob to Flip >");
        }
        else {
            // PAGE 2: 电气与通讯
            String vStr = (d.busVoltage == 0xFFFF) ? "COMM ERR" :
                          String(d.busVoltage) + " V";

            userInterface->displayDiagnosticInfo("Electrical [3/3]",
                "Bus Volt: " + vStr + "\n" +
                "Station:  1\n"
                "Baud:     " + String(MODBUS_BAUD_RATE) + "\n" +
                "  < Knob to Flip >");
        }
    }
};

#endif // SERVO_MONITOR_HANDLER_H
