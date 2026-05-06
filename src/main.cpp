#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include "main.h"
#include "config.h"
#include "user_interface/user_interface.h"
#include "user_interface/drv_oled_rotary/oled.h"
#include "user_interface/drv_terminal/terminal.h"
#include "user_interface/drv_oled_rotary/menu_system.h"
#include "user_interface/drv_rs485_screen/rs485_touch_screen.h"
#include "user_interface/drv_mcgs/mcgs_display.h"
#include "modular/encoder.h"
#include "modular/sorter.h"
#include "handlers/scanner_diagnostic_handler.h"
#include "handlers/outlet_diagnostic_handler.h"
#include "handlers/encoder_diagnostic_handler.h"
#include "handlers/config_handler.h"
#include "handlers/hmi_diagnostic_handler.h"

#include "user_interface/common/menu_config.h"
#include "system/system_manager.h"

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
PhaseOffsetConfigHandler phaseOffsetConfigHandler(userInterface, &sorter);

int normalModeSubmode = 0;
bool hasVersionInfoDisplayed = false;
String systemName = "Feng's AS-L9";

// FreeRTOS 任务句柄
TaskHandle_t hControlTask = nullptr;
TaskHandle_t hUITask = nullptr;

// 任务函数声明
void vControlTask(void* pvParameters);
void vUITask(void* pvParameters);

// ── 初始化子函数 ────────────────────────────────────────────────────────

/**
 * @brief 配置 HMI 硬件驱动并注入到 UI 抽象层
 */
void setupHmi() {
    userInterface->clearAllDisplayDevices();
    userInterface->clearAllInputSources();

    #if CURRENT_HMI_TYPE == 1 // TERMINAL
        Terminal* term = Terminal::getInstance();
        term->initialize();
        userInterface->addDisplayDevice(term);
        userInterface->enableOutputChannel(OUTPUT_SERIAL);
        Serial.println("[BOOT] HMI: TERMINAL");

    #elif CURRENT_HMI_TYPE == 2 // OLED_ROTARY
        OLED* oled = OLED::getInstance();
        oled->initialize();
        userInterface->addDisplayDevice(oled);
        
        RotaryInputSource* rotary = RotaryInputSource::getInstance();
        rotary->initialize();
        userInterface->addInputSource(rotary);
        
        userInterface->enableOutputChannel(OUTPUT_OLED);
        Serial.println("[BOOT] HMI: OLED_ROTARY");

    #elif CURRENT_HMI_TYPE == 3 // MCGS
        static McgsDisplay mcgs;
        mcgs.initialize();
        userInterface->addDisplayDevice(&mcgs);
        userInterface->addInputSource(&mcgs);
        Serial.println("[BOOT] HMI: MCGS");

    #elif CURRENT_HMI_TYPE == 4 // LVGL_TOUCHSCREEN
        Rs485TouchScreen* hmiScreen = Rs485TouchScreen::getInstance();
        hmiScreen->initialize();
        userInterface->addDisplayDevice(hmiScreen);
        userInterface->addInputSource(hmiScreen);
        Serial.println("[BOOT] HMI: RS485_TOUCHSCREEN");
        
    #else
        Serial.println("[BOOT] HMI: NONE");
    #endif

    userInterface->initialize(); // 初始化 UI 逻辑状态
}

/**
 * @brief 初始化 EEPROM 并加载系统持久化数据
 */
void setupSystemData() {
    EEPROM.begin(512);

    // 1. 处理开机计数
    EEPROM.get(EEPROM_ADDR_BOOT_COUNT, systemBootCount);
    if (systemBootCount == 0xFFFFFFFF) systemBootCount = 0;
    systemBootCount++;
    EEPROM.put(EEPROM_ADDR_BOOT_COUNT, systemBootCount);
    EEPROM.commit();
    Serial.printf("[BOOT] System Boot Count: %u\n", systemBootCount);

    // 2. 加载相位偏移 (Phase Offset)
    uint8_t phaseOffsetMagic = EEPROM.read(EEPROM_ADDR_PHASE_OFFSET);
    uint8_t phaseOffsetValue = EEPROM.read(EEPROM_ADDR_PHASE_OFFSET + 1);
    uint8_t savedOffset = 0;
    if (phaseOffsetMagic == 0xA5 && phaseOffsetValue < ENCODER_MAX_PHASE) {
        savedOffset = phaseOffsetValue;
    }
    encoder->setPhaseOffset(savedOffset);
    Serial.printf("[BOOT] Phase Offset Loaded: %d\n", savedOffset);

    // 3. 加载托盘历史数据
    traySystem->loadFromEEPROM(EEPROM_ADDR_TRAY_DATA);
}

/**
 * @brief 初始化核心业务模块与诊断处理器
 */
void setupModules() {
    // 初始化分拣执行器与传感器
    encoder->initialize();
    sorter.initialize();
    diameterScanner->initialize();
    
    // 初始化诊断处理器（注入 UI 引用）
    outletDiagnosticHandler.initialize(userInterface);
    encoderDiagnosticHandler.initialize(userInterface);
    
    // 为诊断处理器绑定出口引用
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        outletDiagnosticHandler.setOutlet(i, sorter.getOutlet(i));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Feng's Sorter System Booting ---");
    
    // 1. 硬件/驱动注入
    setupHmi();
    
    // 2. 数据与持久化
    setupSystemData();
    
    // 3. 业务模块初始化
    setupModules();
    
    // 4. 菜单树构建
    setupMenuTree();
    
    Serial.println("[BOOT] All systems ready\n");

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
        if (currentMode == MODE_NORMAL || currentMode == MODE_DIAGNOSE_OUTLET || currentMode == MODE_DIAGNOSE_SCANNER) {
            sorter.run();
        }
        
        // 保持 1ms 的确定性节拍
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void vUITask(void* pvParameters) {
    Serial.println("[FreeRTOS] UITask (Core 0) started.");

    for (;;) {
        // ── 意图驱动输入 ──
        // 自动轮询所有已注册的输入源 (如物理旋钮、MCGS 触控、串口指令)
        UIIntent intent = userInterface->getNextIntent();
        
        // 映射为旧逻辑兼容变量
        int delta = (intent.action == UIAction::NAVIGATE_RELATIVE) ? intent.value : 0;
        bool btnPressed = (intent.action == UIAction::ACTIVATE);
        bool backPressed = (intent.action == UIAction::BACK);
        
        // --- 核心同步：处理导航路径意图 (如来自从机切页) ---
        if (intent.action == UIAction::NAVIGATE_PATH) {
            switchToMode((SystemMode)intent.value);
        }
        
        uint32_t currentMs = millis();
        
        if (menuModeActive) {
            bool needsRefresh = false;
            
            if (intent.action == UIAction::NAVIGATE_RELATIVE) {
                menuSystem.handleInput(intent.value, false);
                needsRefresh = true;
            } else if (intent.action == UIAction::ACTIVATE) {
                menuSystem.handleInput(0, true);
                if (menuModeActive) needsRefresh = true;
            } else if (intent.action == UIAction::BACK) {
                // 返回逻辑
                handleReturnToMenu();
                needsRefresh = true;
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
                if (btnPressed || backPressed) {
                    handleReturnToMenu();
                } else {
                    switch (currentMode) {
                        case MODE_NORMAL:
                            if (delta != 0) normalModeSubmode = (normalModeSubmode + 1) % 2;
                            break;
                        default:
                            break;
                    }
                }
            }

            static uint32_t lastSnapshotMs = 0;
            if (currentMs - lastSnapshotMs >= 100) {
                lastSnapshotMs = currentMs;

                // ── 渲染与刷新快照 (中央 Broker 广播机制) ──
                DisplaySnapshot snapshot;
                snapshot.currentMode = currentMode;
                
                if (activeHandler) {
                    activeHandler->captureSnapshot(snapshot);
                } else {
                    if (currentMode == MODE_NORMAL) {
                        strcpy(snapshot.activePage, "Dashboard");
                        float speed = sorter.getConveyorSpeedPerSecond();
                        snapshot.data.dashboard.sortingSpeedPerSecond = speed;
                        snapshot.data.dashboard.sortingSpeedPerMinute = (int)(speed * 60.0f);
                        snapshot.data.dashboard.sortingSpeedPerHour = (int)(speed * 3600.0f);
                        snapshot.data.dashboard.identifiedCount = traySystem->getTotalIdentifiedItems();
                        snapshot.data.dashboard.transportedTrayCount = traySystem->getTransportedTrayCount();
                        snapshot.data.dashboard.latestDiameter = sorter.getLatestDiameter();
                        snapshot.data.dashboard.latestScanCount = traySystem->getTrayScanCount(0);
                    } else if (currentMode == MODE_VERSION_INFO) {
                        strcpy(snapshot.activePage, "About");
                    }
                }
                userInterface->refreshAllDevices(snapshot);
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

void handleReturnToMenu() {
    switchToMode(MODE_NORMAL);
    menuModeActive = true;
    hasVersionInfoDisplayed = false;
    
    Serial.println("[MENU] Returned to Main Menu");
    userInterface->clearDisplay();
    userInterface->renderMenu(menuSystem.getCurrentNode(), menuSystem.getCursorIndex(), menuSystem.getScrollOffset());
}


//  测量直径很稳定的版本了
