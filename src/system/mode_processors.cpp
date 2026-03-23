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
    static bool initialized = false;
    
    if (!initialized) {
        initialized = true;
        Serial.println("[NORMAL] Normal Mode Activated (Unified Dashboard)");
    }
    
    // 从 Sorter 和 TraySystem 获取实时数据
    float speedPerSecond = sorter.getConveyorSpeedPerSecond();
    int speedPerMinute = speedPerSecond * 60.0f;
    int speedPerHour = speedPerSecond * 3600.0f;
    
    int identifiedCount = DiameterScanner::getInstance()->getTotalObjectCount();
    const int pulsesPerTray = 200;
    long encoderPosition = encoder->getRawCount();
    int transportedTrayCount = encoderPosition / pulsesPerTray;
    
    // 获取最新一根物料的详细数据
    int latestDiameter = traySystem->getTrayDiameter(0);
    int latestScanCount = traySystem->getTrayScanCount(0);
    
    // 调用功能增强版的仪表盘，设置 forceRefresh 为 true，由 UITask 控制刷新节奏
    UserInterface::getInstance()->displayDashboard(
        speedPerSecond, 
        speedPerMinute, 
        speedPerHour, 
        identifiedCount, 
        transportedTrayCount,
        latestDiameter,
        latestScanCount,
        true // 强制刷新，因为我们在 30Hz 的 UITask 中循环
    );
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
