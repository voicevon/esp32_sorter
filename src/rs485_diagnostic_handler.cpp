#include "rs485_diagnostic_handler.h"
#include "config.h"
#include "modbus_controller.h"

RS485DiagnosticHandler::RS485DiagnosticHandler() : 
    userInterface(nullptr), 
    sorter(nullptr), 
    lastSendTime(0), 
    counter(0), 
    lastReceivedByte(0), 
    hasReceived(false),
    lastDisplayUpdateTime(0) {}

void RS485DiagnosticHandler::initialize(UserInterface* ui, Sorter* s) {
    userInterface = ui;
    sorter = s;
}

void RS485DiagnosticHandler::begin() {
    lastSendTime = millis();
    counter = 0;
    hasReceived = false;
    lastDisplayUpdateTime = 0;
    if (userInterface) {
        userInterface->clearDisplay();
        userInterface->displayDiagnosticValues("RS485 Diag", "Initializing...", "");
    }
    if (sorter) {
    }
    Serial.println("[RS485] Diagnostic Mode Started");
}

void RS485DiagnosticHandler::update(uint32_t currentTime, bool btnPressed) {
    if (btnPressed) {
        handleReturnToMenu();
        return;
    }
    
    static int pollIdx = 0;
    static uint16_t pollAddrs[] = {0x0000, 0x0004, 0x0035, 0x0018, 0x0016, 0x0002, 0x0028, 0x0029}; // PA0, PA4, PA53, PA24, PA22, PA2, PA40, PA41
    static uint16_t pollVals[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    // 1. 发送逻辑 (每 300ms 轮询一个寄存器，加快整体刷新速度)
    if (currentTime - lastSendTime >= 300) {
        lastSendTime = currentTime;
        pollIdx = (pollIdx + 1) % 8;
        
        // 执行轮询逻辑
        ModbusController::getInstance()->requestRegisterRead(pollAddrs[pollIdx]);
    }
    
    // 2. 接收逻辑 (解析 Modbus 0x03 响应)
    static uint8_t rxBuf[32];
    static int rxIdx = 0;
    static unsigned long lastRxTime = 0;
    
    if (rxIdx > 0 && currentTime - lastRxTime > 100) rxIdx = 0;

    while (Serial1.available()) {
        uint8_t b = Serial1.read();
        lastRxTime = currentTime;
        if (rxIdx < 32) rxBuf[rxIdx++] = b;
        
        if (rxIdx >= 7 && rxBuf[0] == 0x01 && rxBuf[1] == 0x03) {
            hasReceived = true;
            uint16_t val = (rxBuf[3] << 8) | rxBuf[4];
            pollVals[pollIdx] = val;
            rxIdx = 0;
        } else if (rxIdx >= 5 && rxBuf[0] == 0x01 && (rxBuf[1] & 0x80)) {
            Serial.printf("[RS485 ERROR] Slave error 0x%02X at addr 0x%04X\n", rxBuf[2], pollAddrs[pollIdx]);
            rxIdx = 0;
        }
    }
    
    // 3. 显示更新逻辑 (每 200ms 更新一次 OLED)
    if (currentTime - lastDisplayUpdateTime >= 200) {
        lastDisplayUpdateTime = currentTime;
        
        // 合并 PA40 和 PA41 得到 32 位位置值
        long posValue = ((uint32_t)pollVals[6] << 16) | pollVals[7];
        
        String line1 = "M(PA4):" + String(pollVals[1]) + " E(PA53):" + String(pollVals[2]);
        String line2 = "Src(PA22):  " + String(pollVals[4]);
        String line3 = "PPR(PA2):   " + String(pollVals[5]);
        String line4 = "Pos:        " + String(posValue);
        String line5 = "Alarm(PA0): " + String(pollVals[0]);
        
        if (userInterface) {
            userInterface->displayMultiLineText("RS485 Debug", line1, line2, line3, line4, line5);
        }
    }
}
