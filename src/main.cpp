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

#include "menu_config.h"
#include "servo_monitor_handler.h"
#include "servo_control_handler.h"
#include "utils/potentiometer.h"
#include "modbus_controller.h"
#include "servo_manager.h"
#include "system_manager.h"
#include "mode_processors.h"

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
ServoConfigHandler servoConfigHandler(userInterface, &sorter);
RS485DiagnosticHandler rs485DiagnosticHandler;
ServoMonitorHandler servoMonitorHandler(userInterface);
ServoControlHandler servoSpeedKnobHandler(userInterface, CTRL_SPEED_KNOB);
ServoControlHandler servoSpeedPotHandler(userInterface, CTRL_SPEED_POT);
ServoControlHandler servoTorqueHandler(userInterface, CTRL_TORQUE_KNOB);

int normalModeSubmode = 0;
bool hasVersionInfoDisplayed = false;
String systemName = "Feng's AS-L9";
Potentiometer pot(PIN_POTENTIOMETER);
unsigned long lastPotSampleTime = 0;

void updateSpeedFromPot(uint32_t currentMs) {
    if (currentMs - lastPotSampleTime >= 200) {
        lastPotSampleTime = currentMs;
        pot.update();
        int targetSpeed = map(pot.getSmoothedValue(), 0, 4095, 0, 600) - 10;
        if (targetSpeed < 0) targetSpeed = 0;
        
        // 使用单例管理者统一调度
        ServoManager::getInstance().setTargetCommand(targetSpeed);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Feng's Sorter system starting...");
    
    userInterface->initialize();
    OLED::getInstance()->initialize();
    Terminal::getInstance()->initialize();
    pot.initialize();
    userInterface->enableOutputChannel(OUTPUT_ALL);
    
    delay(500);
    userInterface->displayDiagnosticValues("Configuring...", "Servo Manager", "Init...");
    
    // 初始化伺服管理器单例，传入断电保存的加减速与扭矩限制
    servoConfigHandler.loadFromEEPROM(); 
    ServoManager::getInstance().begin();
    // 注入初始配置
    ServoManager::getInstance().setTargetMode(1); // 默认速度模式
    
    userInterface->displayDiagnosticValues("Servo Init", "Assigned", "Ready!");
    delay(500);
    
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
    
    // 伺服状态机全局轮询 (处理状态迁移与监控)
    ServoManager::getInstance().update();

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
            if (currentMode == MODE_DIAGNOSE_ENCODER || currentMode == MODE_SERVO_SPEED_POTENTIOMETER) {
                updateSpeedFromPot(currentMs);
            }
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
                    
                    updateSpeedFromPot(currentMs);
                    
                    // 移除此处的自动使能逻辑，以保安全
                    // if (millis() - lastModbusSendTime > 1000) { ... }
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
                case MODE_VERSION_INFO:
                    processVersionInfoMode();
                    break;
                default:
                    break;
            }
        }
    }
}
