#ifndef HMI_DIAGNOSTIC_HANDLER_H
#define HMI_DIAGNOSTIC_HANDLER_H

#include "base_diagnostic_handler.h"
#include "user_interface/user_interface.h"
#include "user_interface/simple_hmi.h"

class HMIDiagnosticHandler : public BaseDiagnosticHandler {
private:
    UserInterface* userInterface;
    long totalRawPulses;
    long totalLogicalUnits;
    unsigned long lastDisplayTime;
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
    
    void update(unsigned long currentTime) override {
        // 实时获取原始脉冲（不影响全局逻辑累加，因为这个模式下我们只看原始值）
        // 实际上 SimpleHMI 的单例 getEncoderDelta 会清零累计值。
        // 为了不破坏 normal 逻辑，我们专门增加一个 getRawEncoderDelta。
        
        SimpleHMI* hmi = SimpleHMI::getInstance();
        int rawDelta = hmi->getRawEncoderDelta();
        if (rawDelta != 0) {
            totalRawPulses += rawDelta;
            // 这里的逻辑单位我们假定是 1:4 (物理格常见比例) 或者当前设置的比例
            // 我们直接显示原始脉冲，让用户自己看 1 格跳几个。
        }

        if (currentTime - lastDisplayTime >= 100) {
            lastDisplayTime = currentTime;
            
            String val1 = "Raw:" + String(totalRawPulses) + " | Err:" + String(hmi->getIllegalTransitionCount());
            String val2 = "Steps(1:4):" + String(totalRawPulses / 4) + " | (1:2):" + String(totalRawPulses / 2);
            
            userInterface->displayDiagnosticValues("HMI Encoder Diag", val1, val2);
        }
    }
};

#endif
