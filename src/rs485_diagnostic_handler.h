#pragma once

#include <Arduino.h>
#include "main.h"
#include "user_interface/user_interface.h"
#include "modular/sorter.h"
#include "base_diagnostic_handler.h"

/**
 * @class RS485DiagnosticHandler
 * @brief Handles the RS485 diagnostic mode logic.
 * 
 * This class manages the transmission of a counter byte every second
 * and the reception of data from the RS485 interface, which is then
 * displayed on the system LEDs and OLED.
 */
class RS485DiagnosticHandler : public BaseDiagnosticHandler {
private:
    UserInterface* userInterface;
    Sorter* sorter;
    
    unsigned long lastSendTime;
    int counter;
    uint8_t lastReceivedByte;
    bool hasReceived;
    
    unsigned long lastDisplayUpdateTime;
    
public:
    RS485DiagnosticHandler();
    void initialize(UserInterface* ui, Sorter* s);
    
    // 实现基类接口
    void begin() override;
    void update(unsigned long currentTime) override;
};
