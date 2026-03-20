#include "handlers/servo_monitor_handler.h"
#include "handlers/servo_control_handler.h"
#include "system_manager.h"
#include "../config.h"
#include "user_interface/user_interface.h"
#include "modular/sorter.h"
#include "servo/modbus_controller.h"
#include "handlers/scanner_diagnostic_handler.h"
#include "handlers/outlet_diagnostic_handler.h"
#include "handlers/encoder_diagnostic_handler.h"
#include "handlers/rs485_diagnostic_handler.h"
#include "handlers/config_handler.h"
#include "handlers/hmi_diagnostic_handler.h"
#include "handlers/base_diagnostic_handler.h"
#include "servo/servo_manager.h"
#include <EEPROM.h>
#include "modular/tray_system.h"

// 全局变量定义
SystemMode currentMode = MODE_NORMAL;
SystemMode pendingMode = MODE_NORMAL;
bool modeChangePending = false;
unsigned long systemBootCount = 0;
String firmwareVersion = "ver: 2601";
BaseDiagnosticHandler* activeHandler = nullptr;

// 外部引用（由 main.cpp 或其他模块定义）
extern MenuSystem menuSystem;
extern bool menuModeActive;
extern Sorter sorter;
extern DiameterConfigHandler diameterConfigHandler;
extern RS485DiagnosticHandler rs485DiagnosticHandler;
extern ScannerDiagnosticHandler scannerDiagnosticHandler;
extern OutletDiagnosticHandler outletDiagnosticHandler;
extern EncoderDiagnosticHandler encoderDiagnosticHandler;
extern HMIDiagnosticHandler hmiDiagnosticHandler;
extern ServoConfigHandler servoConfigHandler;

extern ServoMonitorHandler servoMonitorHandler;
extern ServoControlHandler servoSpeedKnobHandler;
extern ServoControlHandler servoSpeedPotHandler;
extern ServoControlHandler servoTorqueHandler;

void switchToMode(SystemMode mode) {
    pendingMode = mode;
    modeChangePending = true;
    menuModeActive = false;
}

void handleReturnToMenu() {
    if (activeHandler) {
        activeHandler->end();
        activeHandler = nullptr;
    }
    menuModeActive = true;
    UserInterface::getInstance()->resetDiagnosticMode();
    
    Serial.println("[MENU] Returned to Main Menu");
    UserInterface::getInstance()->clearDisplay();
    UserInterface::getInstance()->renderMenu(menuSystem.getCurrentNode(), menuSystem.getCursorIndex(), menuSystem.getScrollOffset());
}

void handleModeChange() {
    if (!modeChangePending) return;

    if (currentMode == MODE_DIAGNOSE_SCANNER) {
        UserInterface::getInstance()->resetDiagnosticMode();
        Serial.println("[DIAGNOSTIC] 已禁用扫描边缘校准模式");
    }
    
    if (currentMode == MODE_CONFIG_DIAMETER) {
        sorter.saveConfig();
    }
    
    SystemMode oldMode = currentMode;
    currentMode = pendingMode;
    modeChangePending = false;
    
    // 重置并设置新的 activeHandler
    if (activeHandler) activeHandler->end();
    activeHandler = nullptr;

    switch (currentMode) {
        case MODE_DIAGNOSE_RS485:
            activeHandler = &rs485DiagnosticHandler;
            break;
        case MODE_DIAGNOSE_ENCODER:
            activeHandler = &encoderDiagnosticHandler;
            break;
        case MODE_DIAGNOSE_SCANNER:
            activeHandler = &scannerDiagnosticHandler;
            break;
        case MODE_DIAGNOSE_OUTLET:
            activeHandler = &outletDiagnosticHandler;
            break;
        case MODE_DIAGNOSE_HMI:
            activeHandler = &hmiDiagnosticHandler;
            break;
        case MODE_CONFIG_DIAMETER:
            activeHandler = &diameterConfigHandler;
            break;
        case MODE_CONFIG_SERVO:
            activeHandler = &servoConfigHandler;
            break;
        case MODE_SERVO_MONITOR:
            activeHandler = &servoMonitorHandler;
            break;
        case MODE_SERVO_SPEED_ENCODER:
            activeHandler = &servoSpeedKnobHandler;
            break;
        case MODE_SERVO_SPEED_POTENTIOMETER:
            activeHandler = &servoSpeedPotHandler;
            break;
        case MODE_SERVO_TORQUE_KNOB:
            activeHandler = &servoTorqueHandler;
            break;
        default:
            activeHandler = nullptr;
            break;
    }

    if (activeHandler) {
        activeHandler->begin();
    }
    
    if (oldMode == MODE_CONFIG_DIAMETER && currentMode != MODE_CONFIG_DIAMETER) {
        diameterConfigHandler.reset();
    }
    
    if (oldMode == MODE_CONFIG_SERVO && currentMode != MODE_CONFIG_SERVO) {
        servoConfigHandler.reset();
        // 退出配置时，永久同步一次到驱动器
        servoConfigHandler.applyToServo();
    }
    
    // --- 伺服状态机意图同步 ---
    if (currentMode == MODE_SERVO_SPEED_ENCODER || currentMode == MODE_SERVO_SPEED_POTENTIOMETER || currentMode == MODE_DIAGNOSE_ENCODER) {
        ServoManager::getInstance().setTargetMode(1); // 切换至速度模式意图
    }
    else if (currentMode == MODE_SERVO_TORQUE_KNOB) {
        ServoManager::getInstance().setTargetMode(2); // 切换至转矩模式意图
    }
    else if (currentMode == MODE_NORMAL) {
        ServoManager::getInstance().setTargetMode(1); // 生产模式意图
    }
    else if (currentMode == MODE_SERVO_MONITOR) {
        // Monitor 模式下不强制切换，仅观察
    }
    else {
        // 其他非控制模式下，建议将目标设为 0 (位置/空闲)
        // ServoManager::getInstance().setTargetMode(0); 
        ServoManager::getInstance().setTargetCommand(0); // 确保不转
    }
    
    // 获取模式名称的逻辑移动到 mode_processors
    extern String getSystemModeName(SystemMode mode);
    Serial.print("[DIAGNOSTIC] Mode switched to: ");
    Serial.println(getSystemModeName(currentMode));
}

void checkPowerLoss() {
    int powerValue = analogRead(PIN_POWER_MONITOR);
    if (powerValue < POWER_LOSS_ADC_THRESHOLD) {
        Serial.println("!!! POWER LOSS DETECTED !!!");
        TraySystem::getInstance()->saveToEEPROM(EEPROM_ADDR_TRAY_DATA);
        EEPROM.commit();
        while (true) delay(10);
    }
}
