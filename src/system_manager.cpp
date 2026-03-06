#include "system_manager.h"
#include "config.h"
#include "user_interface/user_interface.h"
#include "modular/sorter.h"
#include "modbus_controller.h"
#include "scanner_diagnostic_handler.h"
#include "rs485_diagnostic_handler.h"
#include "config_handler.h"
#include <EEPROM.h>

// 全局变量定义
SystemMode currentMode = MODE_NORMAL;
SystemMode pendingMode = MODE_NORMAL;
bool modeChangePending = false;
unsigned long systemBootCount = 0;
String firmwareVersion = "ver: 2601";

// 外部引用（由 main.cpp 或其他模块定义）
extern MenuSystem menuSystem;
extern bool menuModeActive;
extern Sorter sorter;
extern DiameterConfigHandler diameterConfigHandler;
extern RS485DiagnosticHandler rs485DiagnosticHandler;

void switchToMode(SystemMode mode) {
    pendingMode = mode;
    modeChangePending = true;
    menuModeActive = false;
}

void handleReturnToMenu() {
    menuModeActive = true;
    UserInterface::getInstance()->resetDiagnosticMode();
    sorter.clearTestLedByte();
    // 安全退出：返回主菜单时强制停转电机
    ModbusController::getInstance()->setSpeed(0);
    
    Serial.println("[MENU] Returned to Main Menu (Motor STOPPED)");
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
        Serial.println("[CONFIG] Saving configuration to EEPROM...");
        EEPROM.begin(512);
        const int EEPROM_DIAMETER_RANGES_ADDR = 0;
        EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR, 0xAA);
        for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
            EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2, sorter.getOutletMinDiameter(i));
            EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2 + 1, sorter.getOutletMaxDiameter(i));
        }
        EEPROM.commit();
        Serial.println("[CONFIG] Configuration saved to EEPROM");
    }
    
    SystemMode oldMode = currentMode;
    currentMode = pendingMode;
    modeChangePending = false;
    
    if (currentMode == MODE_DIAGNOSE_RS485) {
        rs485DiagnosticHandler.initializeDiagnosticMode();
    }
    
    if (oldMode == MODE_CONFIG_DIAMETER && currentMode != MODE_CONFIG_DIAMETER) {
        diameterConfigHandler.reset();
    }
    
    // --- 伺服同步逻辑状态机 ---
    bool needsSpeedModeSync = false;
    bool forceSync = false;

    if (currentMode == MODE_SERVO_SPEED_ENCODER || currentMode == MODE_SERVO_SPEED_POTENTIOMETER) {
        needsSpeedModeSync = true;
        forceSync = true; // 进入测试，强制响声确认
    }
    else if (currentMode == MODE_NORMAL && oldMode != MODE_NORMAL) {
        needsSpeedModeSync = true;
        forceSync = false; // 返回主模式，智能静默同步
    }

    // 2. 执行同步逻辑
    if (needsSpeedModeSync) {
        ModbusController::getInstance()->syncParameters(true, forceSync);
    }
    
    // 3. 模式退出时的特定清理逻辑
    if ((oldMode == MODE_SERVO_SPEED_ENCODER || oldMode == MODE_SERVO_SPEED_POTENTIOMETER) && currentMode != oldMode) {
        ModbusController::getInstance()->setSpeed(0);
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
