#include "system_manager.h"
#include "../config.h"
#include "modular/sorter.h"
#include "apps/app_scanner_diag.h"
#include "apps/app_outlet_diag.h"
#include "apps/app_encoder_diag.h"
#include "apps/app_config.h"
#include "apps/app_hmi_diag.h"
#include "apps/app_base.h"
#include "apps/app_production.h"
#include <EEPROM.h>

// 前向声明
String getSystemModeName(SystemMode mode);

// 外部引用（由 main.cpp 或其他模块定义）
extern class AppProduction appProduction;
extern Sorter sorter;
extern class AppConfigDiameter appConfigDiameter;
extern class AppConfigPhaseOffset appConfigPhaseOffset;

// 全局变量定义
SystemMode currentMode = MODE_NORMAL;
SystemMode pendingMode = MODE_NORMAL;
bool modeChangePending = false;
unsigned long systemBootCount = 0;
String firmwareVersion = "ver: 2601";
AppBase* activeApp = &appProduction;
extern class AppScannerDiag appScannerDiag;
extern class AppOutletDiag appOutletDiag;
extern class AppEncoderDiag appEncoderDiag;
extern class AppHmiDiag appHmiDiag;
extern bool hasVersionInfoDisplayed;

void switchToMode(SystemMode mode) {
    pendingMode = mode;
    modeChangePending = true;
}

void handleModeChange() {
    if (!modeChangePending) return;

    if (currentMode == MODE_DIAGNOSE_SCANNER) {
        Serial.println("[DIAGNOSTIC] 已禁用扫描边缘校准模式");
    }
    
    if (currentMode == MODE_CONFIG_DIAMETER) {
        sorter.saveConfig();
    }
    
    SystemMode oldMode = currentMode;
    currentMode = pendingMode;
    modeChangePending = false;
    
    // 重置并设置新的 activeApp
    if (activeApp) activeApp->end();
    activeApp = nullptr;

    switch (currentMode) {
        case MODE_NORMAL:
            activeApp = &appProduction;
            break;
        case MODE_DIAGNOSE_ENCODER:
            activeApp = &appEncoderDiag;
            break;
        case MODE_DIAGNOSE_SCANNER:
            activeApp = &appScannerDiag;
            break;
        case MODE_DIAGNOSE_OUTLET:
            activeApp = &appOutletDiag;
            break;
        case MODE_DIAGNOSE_HMI:
            activeApp = &appHmiDiag;
            break;
        case MODE_CONFIG_DIAMETER:
            activeApp = &appConfigDiameter;
            break;
        case MODE_CONFIG_PHASE_OFFSET:
            activeApp = &appConfigPhaseOffset;
            break;
        default:
            activeApp = nullptr;
            break;
    }

    if (activeApp) {
        activeApp->begin();
    }
    
    if (oldMode == MODE_CONFIG_DIAMETER && currentMode != MODE_CONFIG_DIAMETER) {
        appConfigDiameter.reset();
    }
    if (oldMode == MODE_CONFIG_PHASE_OFFSET && currentMode != MODE_CONFIG_PHASE_OFFSET) {
        appConfigPhaseOffset.reset();
    }
    
    Serial.print("[DIAGNOSTIC] Mode switched to: ");
    Serial.println(getSystemModeName(currentMode));
}

String getSystemModeName(SystemMode mode) {
    switch (mode) {
        case MODE_NORMAL: return "Normal Mode";
        case MODE_DIAGNOSE_ENCODER: return "Encoder Diag";
        case MODE_DIAGNOSE_SCANNER: return "Scanner Diag";
        case MODE_DIAGNOSE_OUTLET: return "Outlet Diag";
        case MODE_VERSION_INFO: return "Version Info";
        case MODE_CONFIG_DIAMETER: return "Config Diameter";
        case MODE_DIAGNOSE_HMI: return "HMI Encoder Diag";
        case MODE_CONFIG_PHASE_OFFSET: return "Config Phase Offset";
        default: return "Unknown Mode";
    }
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
