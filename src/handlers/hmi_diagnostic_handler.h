#ifndef HMI_DIAGNOSTIC_HANDLER_H
#define HMI_DIAGNOSTIC_HANDLER_H

#include "base_diagnostic_handler.h"
#include "../user_interface/user_interface.h"
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
            
            String line1 = ""; // Spacer
            String line2 = "Raw Pol: " + String(totalRawPulses);
            String line3 = "Log 1:4: " + String(totalRawPulses / 4);
            String line4 = "Log 1:2: " + String(totalRawPulses / 2);
            String line5 = "Errors:  " + String(hmi->getIllegalTransitionCount());
            
            userInterface->displayMultiLineText("HMI Encoder", line1, line2, line3, line4, line5);
        }
    }
};

#endif
