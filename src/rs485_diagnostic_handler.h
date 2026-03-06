#pragma once

#include <Arduino.h>
#include "main.h"
#include "user_interface/user_interface.h"
#include "modular/sorter.h"

/**
 * @class RS485DiagnosticHandler
 * @brief Handles the RS485 diagnostic mode logic.
 * 
 * This class manages the transmission of a counter byte every second
 * and the reception of data from the RS485 interface, which is then
 * displayed on the system LEDs and OLED.
 */
class RS485DiagnosticHandler {
private:
    UserInterface* userInterface;
    Sorter* sorter;
    
    unsigned long lastSendTime;
    uint8_t counter;
    uint8_t lastReceivedByte;
    String receivedMessage; // 存储完整的接收字符串
    String lastRecvDisplayStr; // 用于 OLED 显示的最后一条完整消息
    bool hasReceived;
    
    unsigned long lastDisplayUpdateTime;
    
public:
    RS485DiagnosticHandler();
    void initialize(UserInterface* ui, Sorter* s);
    void initializeDiagnosticMode();
    void update(unsigned long currentTime);
};
