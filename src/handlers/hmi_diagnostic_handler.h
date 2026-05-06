#ifndef HMI_DIAGNOSTIC_HANDLER_H
#define HMI_DIAGNOSTIC_HANDLER_H

#include "base_diagnostic_handler.h"
#include "../user_interface/user_interface.h"
#include "../user_interface/common/display_types.h"
#include "../user_interface/drv_oled_rotary/RotaryInputSource.h"


class HMIDiagnosticHandler : public BaseDiagnosticHandler {
private:
    UserInterface* userInterface;
    long totalRawPulses;
    long totalLogicalUnits;
    uint32_t lastDisplayTime;
    int currentPulses;
    
public:
    HMIDiagnosticHandler(UserInterface* ui) : 
        userInterface(ui), totalRawPulses(0), totalLogicalUnits(0), lastDisplayTime(0), currentPulses(0) {}
    
    void begin() override {
        totalRawPulses = 0;
        totalLogicalUnits = 0;
        lastDisplayTime = 0;
        currentPulses = 0;
        userInterface->clearDisplay();
        Serial.println("[DIAGNOSTIC] HMI Encoder Diagnostic Started");
    }
    
    void update(uint32_t currentTime, bool btnPressed) override {
        if (btnPressed) {
            handleReturnToMenu();
            return;
        }
        
        // 实际上 RotaryInputSource 的单例 getEncoderDelta 会清零累计值。
        // 为了不破坏 normal 逻辑，我们专门增加一个 getRawEncoderDelta。
        
        RotaryInputSource* hmi = RotaryInputSource::getInstance();
        int rawDelta = hmi->getRawEncoderDelta();
        if (rawDelta != 0) {
            totalRawPulses += rawDelta;
            // 这里的逻辑单位我们假定是 1:4 (物理格常见比例) 或者当前设置的比例
            // 我们直接显示原始脉冲，让用户自己看 1 格跳几个。
        }

        if (currentTime - lastDisplayTime >= 100) {
            lastDisplayTime = currentTime;
            // OLED显示已由 snapshot 统一托管
        }
    }

    void captureSnapshot(DisplaySnapshot& snapshot) override {
        snapshot.currentMode = MODE_DIAGNOSE_HMI;
        strcpy(snapshot.activePage, "diag_hmi");
        
        snapshot.data.encoder.raw = totalRawPulses;
        snapshot.data.encoder.corrected = RotaryInputSource::getInstance()->getIllegalTransitionCount(); // abuse corrected for error transitions
        snapshot.data.encoder.logic = totalRawPulses / 4;
        snapshot.data.encoder.zeroCount = 0;
        snapshot.data.encoder.zeroCorrect = 0;
        snapshot.data.encoder.zeroTotal = 0;
        snapshot.data.encoder.offset = 0;
    }
};

#endif
