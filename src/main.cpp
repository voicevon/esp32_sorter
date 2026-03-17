#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#define MAIN_H_SOURCE
#include "main.h"
#include "config.h"
#include "user_interface/user_interface.h"
#include "user_interface/oled.h"
#include "user_interface/terminal.h"
#include "user_interface/menu_system.h"
#include "modular/encoder.h"
#include "modular/sorter.h"
#include "scanner_diagnostic_handler.h"
#include "outlet_diagnostic_handler.h"
#include "encoder_diagnostic_handler.h"
#include "config_handler.h"
#include "rs485_diagnostic_handler.h"
#include "hmi_diagnostic_handler.h"

// 新模块包含
#include "modbus_controller.h"
#include "system_manager.h"
#include "mode_processors.h"
#include "menu_config.h"

// =========================
// 单例实例
// =========================
UserInterface* userInterface = UserInterface::getInstance();
Encoder* encoder = Encoder::getInstance();
DiameterScanner* diameterScanner = DiameterScanner::getInstance();
TraySystem* allTrays = TraySystem::getInstance();
Sorter sorter;

// 诊断处理器
ScannerDiagnosticHandler scannerDiagnosticHandler;
OutletDiagnosticHandler outletDiagnosticHandler;
EncoderDiagnosticHandler encoderDiagnosticHandler;
HMIDiagnosticHandler hmiDiagnosticHandler(UserInterface::getInstance());
DiameterConfigHandler diameterConfigHandler(userInterface, &sorter);
RS485DiagnosticHandler rs485DiagnosticHandler;

// =========================
// 其它全局变量
// =========================
int normalModeSubmode = 0;
bool hasVersionInfoDisplayed = false;
String systemName = "Feng's AS-L9";
// lastModbusSendTime is defined in modbus_controller.cpp

// 电位器平滑滤波
const int POT_FILTER_WINDOW = 10;
int potSamples[POT_FILTER_WINDOW] = {0};
int potSampleIndex = 0;
long potSum = 0;
bool potBufferFull = false;
unsigned long lastPotSampleTime = 0;

int getSmoothedPot() {
    int raw = analogRead(PIN_POTENTIOMETER);
    potSum -= potSamples[potSampleIndex];
    potSamples[potSampleIndex] = raw;
    potSum += raw;
    potSampleIndex = (potSampleIndex + 1) % POT_FILTER_WINDOW;
    if (potSampleIndex == 0) potBufferFull = true;
    if (!potBufferFull) return raw;
    return potSum / POT_FILTER_WINDOW;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Feng's Sorter system starting...");
    
    userInterface->initialize();
    OLED::getInstance()->initialize();
    Terminal::getInstance()->initialize();
    userInterface->addDisplayDevice(OLED::getInstance());
    userInterface->addDisplayDevice(Terminal::getInstance());
    userInterface->enableOutputChannel(OUTPUT_ALL);
    
    pinMode(PIN_POWER_MONITOR, INPUT);
    ModbusController::getInstance()->initialize();
    pinMode(PIN_POTENTIOMETER, INPUT);

    delay(500);
    userInterface->displayDiagnosticValues("Configuring...", "Modbus Syncing", "Wait 2s...");
    ModbusController::getInstance()->syncParameters(true);
    ModbusController::getInstance()->setSpeed(0);
    
    userInterface->displayDiagnosticValues("Servo Init", "Params Synced", "Ready!");
    delay(1000);
    
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        outletDiagnosticHandler.setOutlet(i, sorter.getOutlet(i));
    }
    
    encoderDiagnosticHandler.initialize(userInterface);
    
    EEPROM.begin(512);
    EEPROM.get(EEPROM_ADDR_BOOT_COUNT, systemBootCount);
    if (systemBootCount == 0xFFFFFFFF) systemBootCount = 0;
    systemBootCount++;
    EEPROM.put(EEPROM_ADDR_BOOT_COUNT, systemBootCount);
    EEPROM.commit();
    
    encoder->initialize();
    sorter.initialize();
    allTrays->loadFromEEPROM(EEPROM_ADDR_TRAY_DATA);
    
    outletDiagnosticHandler.initialize(userInterface);
    encoderDiagnosticHandler.initialize(userInterface);
    rs485DiagnosticHandler.initialize(userInterface, &sorter);
    
    setupMenuTree();
    Serial.println("System ready");
}

void loop() {
    int delta = userInterface->getEncoderDelta();
    bool btnPressed = userInterface->isMasterButtonPressed();
    uint32_t currentMs = millis();

    if (menuModeActive) {
        bool needsRefresh = false;
        if (userInterface->isMasterButtonLongPressed()) {
            if (menuSystem.getCurrentNode() != nullptr && menuSystem.getCurrentNode()->parent != nullptr) {
                menuSystem.handleInput(0, false); 
                needsRefresh = true;
            }
        }
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
        if (btnPressed) {
            handleReturnToMenu();
            return;
        }
        
        // 核心更新逻辑：如果存在活动处理器，则调用其 update
        if (activeHandler) {
            activeHandler->update(currentMs);
            // 特定模式需要额外的每帧逻辑
            if (currentMode == MODE_DIAGNOSE_OUTLET) {
                sorter.run();
                if (delta != 0) {
                    outletDiagnosticHandler.handleEncoderInput(delta);
                }
            }
        } else {
            // 处理非处理器托管的模式
            switch (currentMode) {
                case MODE_NORMAL:
                    if (delta != 0) normalModeSubmode = (normalModeSubmode + 1) % 2;
                    processNormalMode();
                    sorter.run();
                    if (millis() - lastModbusSendTime > 1000) {
                        lastModbusSendTime = millis();
                        ModbusController::getInstance()->setEnable(true); 
                        if (ModbusController::getInstance()->getLastSentSpeed() <= 0) {
                            ModbusController::getInstance()->setSpeed(500);
                        } else {
                            ModbusController::getInstance()->setSpeed(ModbusController::getInstance()->getLastSentSpeed());
                        }
                    }
                    break;
                case MODE_DIAGNOSE_POTENTIOMETER:
                    {
                        static unsigned long lp = 0;
                        if (currentMs - lp > 100) {
                            lp = currentMs;
                            userInterface->displayDiagnosticValues("Potentiometer Test", "Pin: 25", "Raw: " + String(analogRead(PIN_POTENTIOMETER)));
                        }
                    }
                    break;
                case MODE_SERVO_SPEED_ENCODER:
                    if (delta != 0) {
                        int newSpeed = constrain(ModbusController::getInstance()->getLastSentSpeed() + (-delta * 50), MODBUS_SPEED_MIN, MODBUS_SPEED_MAX);
                        ModbusController::getInstance()->setSpeed(newSpeed);
                    }
                    {
                        static unsigned long lp = 0;
                        if (currentMs - lp > 500) {
                            lp = currentMs;
                            ModbusController::getInstance()->setEnable(true);
                            userInterface->displayDiagnosticValues("Speed Ctrl (Enc)", "Step: 50rpm", "Set: " + String(ModbusController::getInstance()->getLastSentSpeed()) + " RPM");
                        }
                    }
                    break;
                case MODE_SERVO_SPEED_POTENTIOMETER:
                    if (currentMs - lastPotSampleTime >= 100) {
                        lastPotSampleTime = currentMs;
                        // 根据需求，将最大值映射到600，然后减去10，防止负数，使得最大范围在0-590
                        int rawSpeed = map(getSmoothedPot(), 0, 4095, 0, 600);
                        int targetSpeed = rawSpeed - 10;
                        if (targetSpeed < 0) targetSpeed = 0;
                        
                        // 死区
                        if (abs(targetSpeed - ModbusController::getInstance()->getLastSentSpeed()) > 2) {
                            ModbusController::getInstance()->setSpeed(targetSpeed);
                        }
                    }
                    {
                        static unsigned long lp = 0;
                        if (currentMs - lp > 500) {
                            lp = currentMs;
                            ModbusController::getInstance()->setEnable(true);
                            userInterface->displayDiagnosticValues("Speed Ctrl (Pot)", "Max 590 RPM", "Set: " + String(ModbusController::getInstance()->getLastSentSpeed()) + " RPM");
                        }
                    }
                    break;
                case MODE_CONFIG_DIAMETER:
                    diameterConfigHandler.update();
                    break;
                case MODE_VERSION_INFO:
                    processVersionInfoMode();
                    break;
                default:
                    break;
            }
        }
        handleModeChange();
    }
}
