#include "encoder_diagnostic_handler.h"
#include "../modular/encoder.h"

EncoderDiagnosticHandler::EncoderDiagnosticHandler() {
    encoder = Encoder::getInstance();
    currentSubMode = 0;
    subModeInitialized = false;
    userInterface = nullptr;
}

void EncoderDiagnosticHandler::begin() {
    Serial.println("[DIAGNOSTIC] Encoder Diagnostic Started");
    subModeInitialized = false;
    lastUIDisplayTime = 0;
}

void EncoderDiagnosticHandler::end() {
    Serial.println("[DIAGNOSTIC] Encoder Diagnostic Ended");
}

void EncoderDiagnosticHandler::initialize(UserInterface* ui) {
    userInterface = ui;
}

void EncoderDiagnosticHandler::update(uint32_t currentMs, bool btnPressed) {
    if (btnPressed) {
        if (currentSubMode == 2) {
            handleReturnToMenu();
            return;
        }
        switchToNextSubMode();
        return;
    }

    if (currentMs - lastUIDisplayTime >= UI_REFRESH_INTERVAL) {
        lastUIDisplayTime = currentMs;
        
        long rawCount = encoder->getRawCount();
        long zeroCross = encoder->getZeroCrossCount();
        int rawPosition = rawCount % 400;
        if (rawPosition < 0) rawPosition += 400;
        
        switch (currentSubMode) {
            case 0:
                userInterface->displayDiagnosticValues(
                    "Enc Stats", 
                    "Pos: " + String(rawPosition),
                    "Count: " + String(rawCount)
                );
                break;
            case 1:
                userInterface->displayDiagnosticValues(
                    "Enc Details",
                    "Zero: " + String(zeroCross),
                    "Ready..."
                );
                break;
            case 2:
                userInterface->displayDiagnosticInfo("Encoder Diag", "Status: Ready\nAction: EXIT\n\nClick to return...");
                break;
        }
    }
}

void EncoderDiagnosticHandler::switchToNextSubMode() {
    currentSubMode = (currentSubMode + 1) % 3;
    Serial.println("[DIAGNOSTIC] Encoder Submode: " + String(currentSubMode));
}
