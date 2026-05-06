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
#include "apps/app_about.h"
#include <EEPROM.h>

// 前向声明
String getAppTypeName(AppType appType);

// 外部引用（由 main.cpp 或其他模块定义）
extern class AppProduction appProduction;
extern Sorter sorter;
extern class AppConfigDiameter appConfigDiameter;
extern class AppConfigPhaseOffset appConfigPhaseOffset;
extern AppAbout appAbout;

// 全局变量定义
AppType currentAppType = APP_PRODUCTION;
AppType pendingAppType = APP_PRODUCTION;
bool appTypeChangePending = false;
unsigned long systemBootCount = 0;
String firmwareVersion = "ver: 2601";
AppBase* activeApp = &appProduction;
extern class AppScannerDiag appScannerDiag;
extern class AppOutletDiag appOutletDiag;
extern class AppEncoderDiag appEncoderDiag;
extern class AppHmiDiag appHmiDiag;
extern bool hasVersionInfoDisplayed;

void switchToAppType(AppType appType) {
    pendingAppType = appType;
    appTypeChangePending = true;
}

void handleAppTypeChange() {
    if (!appTypeChangePending) return;

    if (currentAppType == APP_DIAG_SCANNER) {
        Serial.println("[DIAGNOSTIC] 已禁用扫描边缘校准模式");
    }
    
    if (currentAppType == APP_CONFIG_DIAMETER) {
        sorter.saveConfig();
    }
    
    AppType oldAppType = currentAppType;
    currentAppType = pendingAppType;
    appTypeChangePending = false;
    
    // 重置并设置新的 activeApp
    if (activeApp) activeApp->end();
    activeApp = nullptr;

    switch (currentAppType) {
        case APP_PRODUCTION:
            activeApp = &appProduction;
            break;
        case APP_DIAG_ENCODER:
            activeApp = &appEncoderDiag;
            break;
        case APP_DIAG_SCANNER:
            activeApp = &appScannerDiag;
            break;
        case APP_DIAG_OUTLET:
            activeApp = &appOutletDiag;
            break;
        case APP_DIAG_HMI:
            activeApp = &appHmiDiag;
            break;
        case APP_CONFIG_DIAMETER:
            activeApp = &appConfigDiameter;
            break;
        case APP_CONFIG_PHASE_OFFSET:
            activeApp = &appConfigPhaseOffset;
            break;
        case APP_VERSION_INFO:
            activeApp = &appAbout;
            break;
        default:
            activeApp = nullptr;
            break;
    }

    if (activeApp) {
        activeApp->begin();
    }
    
    if (oldAppType == APP_CONFIG_DIAMETER && currentAppType != APP_CONFIG_DIAMETER) {
        appConfigDiameter.reset();
    }
    if (oldAppType == APP_CONFIG_PHASE_OFFSET && currentAppType != APP_CONFIG_PHASE_OFFSET) {
        appConfigPhaseOffset.reset();
    }
    
    Serial.print("[DIAGNOSTIC] App type switched to: ");
    Serial.println(getAppTypeName(currentAppType));
}

String getAppTypeName(AppType appType) {
    switch (appType) {
        case APP_PRODUCTION: return "Normal Mode";
        case APP_DIAG_ENCODER: return "Encoder Diag";
        case APP_DIAG_SCANNER: return "Scanner Diag";
        case APP_DIAG_OUTLET: return "Outlet Diag";
        case APP_VERSION_INFO: return "Version Info";
        case APP_CONFIG_DIAMETER: return "Config Diameter";
        case APP_DIAG_HMI: return "HMI Encoder Diag";
        case APP_CONFIG_PHASE_OFFSET: return "Config Phase Offset";
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
