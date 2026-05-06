#include "app_about.h"
#include <Arduino.h>
#include "../main.h"
#include "../user_interface/common/display_types.h"

extern void handleReturnToMenu();

AppAbout::AppAbout() {}

void AppAbout::begin() {
    Serial.println("[AppAbout] About App Started");
}

void AppAbout::update(uint32_t currentTime, bool btnPressed) {
    if (btnPressed) {
        handleReturnToMenu();
    }
}

void AppAbout::end() {
    Serial.println("[AppAbout] About App Stopped");
}

void AppAbout::captureSnapshot(DisplaySnapshot& snapshot) {
    snapshot.currentMode = APP_VERSION_INFO;
    strcpy(snapshot.activePage, "About");
}
