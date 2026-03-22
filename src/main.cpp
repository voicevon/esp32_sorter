#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include "main.h"
#include "config.h"
#include "user_interface/user_interface.h"
#include "user_interface/oled.h"
#include "user_interface/terminal.h"
#include "user_interface/menu_system.h"
#include "modular/encoder.h"
#include "modular/sorter.h"
#include "handlers/scanner_diagnostic_handler.h"
#include "handlers/outlet_diagnostic_handler.h"
#include "handlers/encoder_diagnostic_handler.h"
#include "handlers/config_handler.h"
#include "handlers/hmi_diagnostic_handler.h"

#include "system/menu_config.h"
#include "system/system_manager.h"
#include "system/mode_processors.h"

// =========================
// 单例实例
// =========================
UserInterface* userInterface = UserInterface::getInstance();
Encoder* encoder = Encoder::getInstance();
DiameterScanner* diameterScanner = DiameterScanner::getInstance();
TraySystem* traySystem = TraySystem::getInstance();
Sorter sorter;

// 诊断处理器
ScannerDiagnosticHandler scannerDiagnosticHandler;
OutletDiagnosticHandler outletDiagnosticHandler;
EncoderDiagnosticHandler encoderDiagnosticHandler;
HMIDiagnosticHandler hmiDiagnosticHandler(UserInterface::getInstance());
DiameterConfigHandler diameterConfigHandler(userInterface, &sorter);

int normalModeSubmode = 0;
bool hasVersionInfoDisplayed = false;
String systemName = "Feng's AS-L9";

void setup() {
    Serial.begin(115200);
    Serial.println("Feng's Sorter system starting...");
    
    userInterface->initialize();
    OLED::getInstance()->initialize();
    Terminal::getInstance()->initialize();
    
    // 注入显示设备
    UserInterface::addExternalDisplayDevice(OLED::getInstance());
    UserInterface::addExternalDisplayDevice(Terminal::getInstance());
    
    userInterface->enableOutputChannel(OUTPUT_ALL);
    
    delay(500);
    
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        outletDiagnosticHandler.setOutlet(i, sorter.getOutlet(i));
    }
    
    EEPROM.begin(512);
    EEPROM.get(EEPROM_ADDR_BOOT_COUNT, systemBootCount);
    if (systemBootCount == 0xFFFFFFFF) systemBootCount = 0;
    systemBootCount++;
    EEPROM.put(EEPROM_ADDR_BOOT_COUNT, systemBootCount);
    EEPROM.commit();
    
    encoder->initialize();
    sorter.initialize();
    diameterScanner->initialize();
    traySystem->loadFromEEPROM(EEPROM_ADDR_TRAY_DATA);
    
    outletDiagnosticHandler.initialize(userInterface);
    encoderDiagnosticHandler.initialize(userInterface);
    
    setupMenuTree();
    Serial.println("System ready");
}

void loop() {
    int delta = userInterface->getEncoderDelta();
    bool btnPressed = userInterface->isMasterButtonPressed();
    uint32_t currentMs = millis();
    
    if (menuModeActive) {
        bool needsRefresh = false;
        if (delta != 0) {
            menuSystem.handleInput(delta, false);
            needsRefresh = true;
        }
        if (btnPressed) {
            menuSystem.handleInput(0, true);
            if (menuModeActive) needsRefresh = true;
        }
        if (needsRefresh) {
            userInterface->renderMenu(menuSystem.getCurrentNode(), menuSystem.getCursorIndex(), menuSystem.getScrollOffset());
        }
    } else {
        handleModeChange(); // 强制模式状态检查

        // 处理工作模式逻辑
        if (activeHandler) {
            // 核心更新逻辑
            activeHandler->update(currentMs, btnPressed);
            
            // 模式特定的附加补丁
            if (currentMode == MODE_DIAGNOSE_OUTLET) {
                sorter.run();
                if (delta != 0) {
                    outletDiagnosticHandler.handleEncoderInput(delta);
                }
            }
        } else {
            // 返回菜单：在独立参数编辑模式下，短按即是“确定并返回列表”
            if (btnPressed) {
                handleReturnToMenu();
                return;
            }
            
            // 处理非处理器托管的模式
            switch (currentMode) {
                case MODE_NORMAL:
                    if (delta != 0) normalModeSubmode = (normalModeSubmode + 1) % 2;
                    processNormalMode();
                    sorter.run();
                    break;
                case MODE_VERSION_INFO:
                    processVersionInfoMode();
                    break;
                default:
                    break;
            }
        }
    }
}
