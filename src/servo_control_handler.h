#ifndef SERVO_CONTROL_HANDLER_H
#define SERVO_CONTROL_HANDLER_H

#include "base_diagnostic_handler.h"
#include "servo_manager.h"
#include "user_interface/user_interface.h"

enum ControlType {
    CTRL_SPEED_KNOB,
    CTRL_SPEED_POT,
    CTRL_TORQUE_KNOB
};

class ServoControlHandler : public BaseDiagnosticHandler {
private:
    UserInterface* userInterface;
    ControlType type;
    uint32_t lastRefreshMs = 0;
    int currentTarget = 0;
    int targetSpeedLimit = 500;
    bool editingLimit = false;

public:
    ServoControlHandler(UserInterface* ui, ControlType t) : userInterface(ui), type(t) {}

    void begin() override {
        currentTarget = 0;
        targetSpeedLimit = 500;
        editingLimit = false;
        lastRefreshMs = 0;

        if (type == CTRL_TORQUE_KNOB) {
            ServoManager::getInstance().setTargetMode(2); 
        } else {
            ServoManager::getInstance().setTargetMode(1);
        }
        ServoManager::getInstance().setTargetCommand(0);

        Serial.printf("[UI] Control Mode Target Set. Monitoring Link...\n");
    }

    void update(uint32_t currentMs, bool btnPressed) override {
        auto& sm = ServoManager::getInstance();
        ServoState state = sm.getState();

        // 状态机未进入运行或异常状态前，展示状态信息
        if (state != SYS_RUNNING && state != SYS_FAULT) {
             userInterface->displayDiagnosticInfo("Servo Init", 
                "State: " + String(sm.getStateName()) + "\n" +
                "Target: Mode " + String(type == CTRL_TORQUE_KNOB ? "2" : "1") + "\n" +
                "Waiting for Ready...");
             return;
        }

        if (btnPressed) {
            if (type == CTRL_TORQUE_KNOB && !editingLimit) {
                editingLimit = true;
            } else {
                sm.setTargetCommand(0);
                handleReturnToMenu();
                return;
            }
        }

        handleInput();

        if (currentMs - lastRefreshMs >= 200 || btnPressed) {
            lastRefreshMs = currentMs;
            refreshUI();
        }
    }

    void end() override {
        ServoManager::getInstance().setTargetCommand(0);
        Serial.println("[UI] Control Mode Exited");
    }

private:
    void handleInput() {
        auto& sm = ServoManager::getInstance();
        int delta = userInterface->getEncoderDelta();

        if (type == CTRL_SPEED_KNOB) {
            if (delta != 0) {
                currentTarget = constrain(currentTarget + (delta * 50), 0, 1500); 
                sm.setTargetCommand(currentTarget);
            }
        } 
        else if (type == CTRL_TORQUE_KNOB) {
            if (delta != 0) {
                if (editingLimit) {
                    targetSpeedLimit = constrain(targetSpeedLimit + (delta * 50), 50, 600);
                    ModbusController::getInstance()->writeRegister(0x009F, (uint16_t)targetSpeedLimit);
                } else {
                    currentTarget = constrain(currentTarget + (delta * 10), -1200, 1200);
                    sm.setTargetCommand(currentTarget);
                }
            }
        }
        else if (type == CTRL_SPEED_POT) {
            int potRaw = analogRead(25); 
            int target = map(potRaw, 0, 4095, 0, 600);
            if (abs(target - currentTarget) > 5) {
                currentTarget = target;
                sm.setTargetCommand(currentTarget);
            }
        }
    }

    void refreshUI() {
        auto& sm = ServoManager::getInstance();
        auto& data = sm.getData();
        
        String title = (type == CTRL_SPEED_KNOB) ? "Speed (Knob)" : 
                       (type == CTRL_SPEED_POT)  ? "Speed (Pot)"  : "Torque Control";

        String info = "State: " + String(sm.getStateName()) + "\n";
        
        if (type != CTRL_TORQUE_KNOB) {
            info += "Tgt: " + String(currentTarget) + " RPM\n" +
                    "Cur: " + String(data.actualSpeed) + " RPM\n" +
                    "Trq: " + String(data.actualTorque/10.0f, 1) + " %\n" +
                    "Pos: " + String(data.position);
        } else {
            String ptrT = (!editingLimit) ? ">" : " ";
            String ptrL = (editingLimit) ? ">" : " ";
            info += ptrT + "Trq Tgt: " + String(currentTarget/10.0f, 1) + " %\n" +
                    ptrL + "Spd Lim: " + String(targetSpeedLimit) + " RPM\n" +
                    "Spd Cur: " + String(data.actualSpeed) + " RPM\n" +
                    "Pos: " + String(data.position);
        }

        userInterface->displayDiagnosticInfo(title, info);
    }
};

#endif // SERVO_CONTROL_HANDLER_H
