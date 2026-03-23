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

// FreeRTOS 任务句柄
TaskHandle_t hControlTask = nullptr;
TaskHandle_t hUITask = nullptr;

// 任务函数声明
void vControlTask(void* pvParameters);
void vUITask(void* pvParameters);

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

    // ==========================================
    // 任务创建：将分拣控制核心与 UI 交互核心物理隔离
    // ==========================================
    
    // 1. 创建控制任务 (Core 1, 高优先级, 1000Hz)
    xTaskCreatePinnedToCore(
        vControlTask,   // 任务函数
        "ControlTask",  // 任务名称
        4096,           // 栈大小
        nullptr,        // 参数
        10,             // 优先级 (最高)
        &hControlTask,  // 句柄
        1               // 绑定到 Core 1 (Arduino 默认核)
    );

    // 2. 创建 UI 任务 (Core 0, 低优先级, 30Hz)
    xTaskCreatePinnedToCore(
        vUITask,        // 任务函数
        "UITask",       // 任务名称
        8192,           // 栈大小
        nullptr,        // 参数
        1,              // 优先级
        &hUITask,       // 句柄
        0               // 绑定到 Core 0
    );
}

void vControlTask(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1); // 1ms 循环频率

    Serial.println("[FreeRTOS] ControlTask (Core 1) started.");

    for (;;) {
        // 分拣逻辑消费执行
        // 只有在 Normal 模式或特定的分拣诊断模式下才运行逻辑处理槽
        if (currentMode == MODE_NORMAL || currentMode == MODE_DIAGNOSE_OUTLET) {
            sorter.run();
        }
        
        // 保持 1ms 的确定性节拍
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void vUITask(void* pvParameters) {
    Serial.println("[FreeRTOS] UITask (Core 0) started.");

    for (;;) {
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

            // 处理工作模式逻辑 (主要由 Handler 更新显示)
            if (activeHandler) {
                activeHandler->update(currentMs, btnPressed);
                
                // 模式特定的附加补丁
                if (currentMode == MODE_DIAGNOSE_OUTLET) {
                    if (delta != 0) {
                        outletDiagnosticHandler.handleEncoderInput(delta);
                    }
                }
            } else {
                if (btnPressed) {
                    handleReturnToMenu();
                } else {
                    switch (currentMode) {
                        case MODE_NORMAL:
                            if (delta != 0) normalModeSubmode = (normalModeSubmode + 1) % 2;
                            processNormalMode();
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
        
        // 给系统任务（如 Watchdog/WiFi）留出时间，并维持约 30Hz 刷新
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// ==========================================
// Arduino 框架要求
// ==========================================
void loop() {
    // 原始 loop() 在双核模式下已废弃，任务由 vControlTask 和 vUITask 承载
    // 我们直接删除这个默认创建的 IDLE 任务以节省资源
    vTaskDelete(NULL);
}
