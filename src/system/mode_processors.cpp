#include "mode_processors.h"
#include "../user_interface/user_interface.h"
#include "../modular/sorter.h"
#include "../modular/encoder.h"
#include "../modular/diameter_scanner.h"
#include "system_manager.h"

// 外部引用
extern Sorter sorter;
extern TraySystem* traySystem;
extern Encoder* encoder;
extern bool hasVersionInfoDisplayed;
extern int normalModeSubmode;

void processVersionInfoMode() {
    static bool hasVersionInfoDisplayedLocal = false;
    if (!hasVersionInfoDisplayedLocal) {
        hasVersionInfoDisplayedLocal = true;
        Serial.println("[VERSION] Version Info Mode Activated");
        String versionInfo = "\n\nAsparagus sorter\n\n";
        versionInfo += "Tel: 133-0640-0990\n";
        versionInfo += "Boot Count: " + String(systemBootCount);
        UserInterface::getInstance()->displayDiagnosticInfo(systemName, versionInfo);
    }
}

void processNormalMode() {
    static bool subModeInitialized = false;
    static int lastPos = -1;
    
    int currentPos = encoder->getCurrentPosition();
    // 只有当编码器正好到达数据锁存相位（170）时强制刷新，其余时间按界面事件刷新
    bool shouldForceRefresh = (currentPos == PHASE_DATA_LATCH && lastPos != PHASE_DATA_LATCH);
    
    if (!subModeInitialized) {
        subModeInitialized = true;
        Serial.println("[NORMAL] Normal Mode Activated");
        shouldForceRefresh = true; // 初次进入强制刷新一次
    }
    lastPos = currentPos;
    
    if (normalModeSubmode == 0) {
        float speedPerSecond = sorter.getConveyorSpeedPerSecond();
        int speedPerMinute = speedPerSecond * 60.0f;
        int speedPerHour = speedPerSecond * 3600.0f;
        int identifiedCount = DiameterScanner::getInstance()->getTotalObjectCount();
        const int pulsesPerTray = 200;
        long encoderPosition = encoder->getRawCount();
        int transportedTrayCount = encoderPosition / pulsesPerTray;
        UserInterface::getInstance()->displayDashboard(speedPerSecond, speedPerMinute, speedPerHour, identifiedCount, transportedTrayCount, shouldForceRefresh);
    } else {
        int latestDiameter = traySystem->getTrayDiameter(0);
        UserInterface::getInstance()->displayNormalModeDiameter(latestDiameter, shouldForceRefresh);
    }
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
        default: return "Unknown Mode";
    }
}
