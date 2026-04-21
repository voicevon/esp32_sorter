#include "system_manager.h"
#include "../config.h"
#include "user_interface/user_interface.h"
#include "modular/sorter.h"
#include "handlers/scanner_diagnostic_handler.h"
#include "handlers/outlet_diagnostic_handler.h"
#include "handlers/encoder_diagnostic_handler.h"
#include "handlers/config_handler.h"
#include "handlers/hmi_diagnostic_handler.h"
#include "handlers/base_diagnostic_handler.h"
#include <EEPROM.h>

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
extern PhaseOffsetConfigHandler phaseOffsetConfigHandler;
extern ScannerDiagnosticHandler scannerDiagnosticHandler;
extern OutletDiagnosticHandler outletDiagnosticHandler;
extern EncoderDiagnosticHandler encoderDiagnosticHandler;
extern HMIDiagnosticHandler hmiDiagnosticHandler;
extern bool hasVersionInfoDisplayed;

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
    hasVersionInfoDisplayed = false;
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
        case MODE_CONFIG_PHASE_OFFSET:
            activeHandler = &phaseOffsetConfigHandler;
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
    if (oldMode == MODE_CONFIG_PHASE_OFFSET && currentMode != MODE_CONFIG_PHASE_OFFSET) {
        phaseOffsetConfigHandler.reset();
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
