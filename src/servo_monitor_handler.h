#ifndef SERVO_MONITOR_HANDLER_H
#define SERVO_MONITOR_HANDLER_H

#include "base_diagnostic_handler.h"
#include "modbus_controller.h"
#include "user_interface/user_interface.h"

class ServoMonitorHandler : public BaseDiagnosticHandler {
private:
    UserInterface* userInterface;
    uint32_t lastRefreshMs = 0;
    int currentPage = 0; // 0: Overview, 1: Live Data, 2: Electrical
    
public:
    ServoMonitorHandler(UserInterface* ui) : userInterface(ui) {}

    void begin() override {
        Serial.println("[DASHBOARD] Servo Monitor Started");
        lastRefreshMs = 0;
        currentPage = 0;
    }

    void update(uint32_t currentMs, bool btnPressed) override {
        if (btnPressed) {
            handleReturnToMenu();
            return;
        }

        // 处理旋钮翻页
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
        Serial.println("[DASHBOARD] Servo Monitor Stopped");
    }

private:
    void refreshDashboard() {
        auto m = ModbusController::getInstance();
        
        if (currentPage == 0) {
            // PAGE 0: 概览 (Overview)
            uint16_t mode  = m->readRegisterSync(0x0004);
            delay(10);
            uint16_t alarm = m->readRegisterSync(0x1013);
            
            String modeStr = "Unknown";
            if (mode == 0) modeStr = "Position";
            else if (mode == 1) modeStr = "Speed";
            else if (mode == 2) modeStr = "Torque";

            String alarmStr = (alarm == 0xFFFF) ? "ERR" : (alarm == 0 ? "Normal" : "ALM-" + String(alarm));

            userInterface->displayDiagnosticInfo("Servo Dashboard [1/3]", 
                "Mode:   " + modeStr + "\n" +
                "Status: " + alarmStr + "\n" +
                "\n" +
                "  < Knob to Flip >");
        } 
        else if (currentPage == 1) {
            // PAGE 1: 实时数据 (Live Data)
            uint16_t speed   = m->readRegisterSync(0x1000);
            delay(15);
            uint16_t torque  = m->readRegisterSync(0x1007);
            delay(15);
            uint16_t current = m->readRegisterSync(0x1008);

            String speedStr  = (speed == 0xFFFF)  ? "ERR" : String((int16_t)speed) + " RPM";
            String torqueStr = (torque == 0xFFFF) ? "ERR" : String((int16_t)torque) + " %";
            String currStr   = (current == 0xFFFF)? "ERR" : String(current / 10.0f, 1) + " A";

            userInterface->displayDiagnosticInfo("Servo Dashboard [2/3]", 
                "Speed:  " + speedStr + "\n" +
                "Torque: " + torqueStr + "\n" +
                "Curr:   " + currStr + "\n" +
                "  < Knob to Flip >");
        }
        else {
            // PAGE 2: 电气与通讯 (Electrical)
            uint16_t busV = m->readRegisterSync(0x1012);
            
            String vStr = (busV == 0xFFFF) ? "COMM ERR" : String(busV) + " V";

            userInterface->displayDiagnosticInfo("Servo Dashboard [3/3]", 
                "Bus Volt: " + vStr + "\n" +
                "Station:  1\n" +
                "Baud:     9600\n" +
                "  < Knob to Flip >");
        }
    }
};

#endif // SERVO_MONITOR_HANDLER_H
