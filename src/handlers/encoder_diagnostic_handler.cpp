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
        long totalZ = encoder->getZeroCrossCount();
        int errorZ = encoder->getForcedZeroCount();
        long lastZRaw = encoder->getZeroCrossRawCount();
        int correctZ = totalZ - errorZ;
        
        int logicalPos = encoder->getCurrentPosition();
        
        switch (currentSubMode) {
            case 0:
                // 基础状态：实时逻辑位置与原始脉冲
                userInterface->displayDiagnosticValues(
                    "Enc Position", 
                    "Logical: " + String(logicalPos),
                    "Raw: " + String(rawCount)
                );
                break;
            case 1:
                // Z相健康度报告：分析同步准确度
                {
                    String line1 = "Correct: " + String(correctZ);
                    String line2 = "Errors : " + String(errorZ);
                    String line3 = "LastZRaw:" + String(lastZRaw);
                    userInterface->displayMultiLineText("Encoder Report", line1, line2, line3, "Z-Signal OK");
                }
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
