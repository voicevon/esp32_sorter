#ifndef POTENTIOMETER_DIAGNOSTIC_HANDLER_H
#define POTENTIOMETER_DIAGNOSTIC_HANDLER_H

#include "base_diagnostic_handler.h"
#include "../user_interface/user_interface.h"
#include "../modular/potentiometer.h"

extern Potentiometer speedPot;

class PotentiometerDiagnosticHandler : public BaseDiagnosticHandler {
private:
    UserInterface* userInterface;
    uint32_t lastDisplayTime;
    
public:
    PotentiometerDiagnosticHandler(UserInterface* ui) : 
        userInterface(ui), lastDisplayTime(0) {}
    
    void begin() override {
        lastDisplayTime = 0;
        userInterface->clearDisplay();
        Serial.println("[DIAGNOSTIC] Potentiometer Diagnostic Started");
    }
    
    void update(uint32_t currentTime, bool btnPressed) override {
        if (btnPressed) {
            handleReturnToMenu();
            return;
        }
        
        speedPot.update();
        
        if (currentTime - lastDisplayTime >= 100) {
            lastDisplayTime = currentTime;
            
            String line1 = "";
            String line2 = "Raw:    " + String(speedPot.getRawValue());
            String line3 = "Filter: " + String(speedPot.getSmoothedValue());
            
            userInterface->displayMultiLineText("Potentiometer Test", line1, line2, line3, "", "");
        }
    }
};

#endif
