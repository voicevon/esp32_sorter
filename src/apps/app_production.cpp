#include "app_production.h"
#include "../modular/sorter.h"
#include "../modular/tray_system.h"
#include "../modular/encoder.h"
#include "../user_interface/user_interface.h"

extern Sorter sorter;
extern int normalModeSubmode;

AppProduction::AppProduction() : _subMode(0) {}

void AppProduction::begin() {
    Serial.println("[AppProduction] Production App Started");
}

void AppProduction::update(uint32_t currentTime, bool btnPressed) {
    if (btnPressed) {
        handleReturnToMenu();
        return;
    }

    int delta = UserInterface::getInstance()->getRawEncoderDelta();
    if (delta != 0) {
        normalModeSubmode = (normalModeSubmode + 1) % 2;
        Serial.printf("[AppProduction] Knob rotated, normal submode: %d\n", normalModeSubmode);
    }
}

void AppProduction::end() {
    Serial.println("[AppProduction] Production App Stopped");
}

void AppProduction::captureSnapshot(DisplaySnapshot& snapshot) {
    snapshot.currentMode = MODE_NORMAL;
    strcpy(snapshot.activePage, "Dashboard");
    
    float speed = sorter.getConveyorSpeedPerSecond();
    snapshot.data.dashboard.sortingSpeedPerSecond = speed;
    snapshot.data.dashboard.sortingSpeedPerMinute = (int)(speed * 60.0f);
    snapshot.data.dashboard.sortingSpeedPerHour = (int)(speed * 3600.0f);
    snapshot.data.dashboard.identifiedCount = TraySystem::getInstance()->getTotalIdentifiedItems();
    snapshot.data.dashboard.transportedTrayCount = TraySystem::getInstance()->getTransportedTrayCount();
    snapshot.data.dashboard.latestDiameter = sorter.getLatestDiameter();
    snapshot.data.dashboard.latestScanCount = TraySystem::getInstance()->getTrayScanCount(0);
}
