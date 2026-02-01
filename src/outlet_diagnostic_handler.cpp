#include "outlet_diagnostic_handler.h"
#include "config.h"

OutletDiagnosticHandler::OutletDiagnosticHandler() {
    // 初始化诊断模式状态变量
    modeStartTime = 0;
    lastOutletTime = 0;
    outletState = false;
    currentOutlet = 0;
    displayInitialized = false;
    currentSubMode = 0;
    cycleCount = 0;
    userInterface = nullptr;
    
    // 初始化outlets数组为nullptr
    for (int i = 0; i < NUM_OUTLETS; i++) {
        outlets[i] = nullptr;
    }
}

void OutletDiagnosticHandler::initialize(UserInterface* ui) {
    // 由于不同模式不会同时运行，这里不再重复初始化硬件资源
    // 出口舵机已在Sorter类的initialize方法中初始化
    
    // 设置UserInterface实例，用于显示诊断信息
    userInterface = ui;
}

void OutletDiagnosticHandler::setOutlet(uint8_t index, Outlet* outlet) {
    if (index < NUM_OUTLETS) {
        outlets[index] = outlet;
    }
}

void OutletDiagnosticHandler::switchToNextSubMode() {
    // 切换子模式（0、1、2之间循环）
    currentSubMode = (currentSubMode + 1) % 3;
    
    // 打印子模式切换信息
    String subModeName;
    switch (currentSubMode) {
        case 0:
            subModeName = "Cycle Drop (Normally Closed)";
            break;
        case 1:
            subModeName = "Cycle Raise (Normally Open)";
            break;
        case 2:
            subModeName = "Servo Lifetime Test";
            break;
        default:
            subModeName = "Unknown";
    }
    Serial.println("[DIAGNOSTIC] Outlet Submode switched: " + subModeName);
    
    // 更新显示内容
    if (userInterface != nullptr) {
        userInterface->displayDiagnosticInfo("Outlet Diag", "SubMode: " + subModeName);
    }
}

void OutletDiagnosticHandler::update(unsigned long currentTime) {
    // 模式开始时初始化
    if (modeStartTime == 0) {
        initializeDiagnosticMode(currentTime);
    }
    
    // 声明局部变量
    unsigned long interval;    // 出口状态保持时间间隔（毫秒）
    String testType;          // 测试类型描述（用于日志输出）
    
    /**
     * 根据子模式执行不同的出口测试逻辑
     * 每个子模式定义了不同的出口开关时序和行为模式
     */
    switch (currentSubMode) {
        case 0: {
            /**
             * 子模式0：轮巡降落测试（常态闭合模式）
             * 行为：出口默认保持关闭状态，定期短暂打开后恢复关闭
             * 时序：
             *   - 关闭状态保持时间：4.5秒（outletState为false时）
             *   - 打开状态保持时间：1.5秒（outletState为true时）
             * 应用场景：测试弹簧降落式出口的正常工作
             */
            interval = outletState ? 1500 : 4500;
            testType = "normally closed";  // 表示出口在非测试状态下是常闭的
            processCycleOperation(currentTime, interval, testType);
            break;
        }
        case 1: {
            /**
             * 子模式1：轮巡上升测试（常态打开模式）
             * 行为：出口默认保持打开状态，定期短暂闭合后恢复打开
             * 时序：
             *   - 打开状态保持时间：4.5秒（outletState为true时）
             *   - 闭合状态保持时间：1.5秒（outletState为false时）
             * 应用场景：测试推杆上升式出口的正常工作
             */
            // 不使用processCycleOperation函数，直接实现逻辑
            if (currentTime - lastOutletTime >= (outletState ? 4500 : 1500)) {
                // 更新时间戳
                lastOutletTime = currentTime;
                
                // 切换出口状态（开<->关）
                outletState = !outletState;
                
                // 当状态从关闭切换到打开时，移动到下一个出口
                if (outletState) {
                    currentOutlet = (currentOutlet + 1) % NUM_OUTLETS;
                    
                    // 输出诊断信息到串口
                    Serial.print("[DIAGNOSTIC] Now testing outlet normally open: ");
                    Serial.println(currentOutlet);
                }
                
                // 直接控制当前出口执行相应动作
                outlets[currentOutlet]->setReadyToOpen(outletState);
                outlets[currentOutlet]->execute();
                
                // 更新显示屏内容，指示当前测试状态
                if (outletState) {
                    // 出口打开时，显示当前测试的出口编号
                    userInterface->displayOutletTestGraphic(NUM_OUTLETS, currentOutlet, currentSubMode);
                } else {
                    // 出口关闭时，显示无出口打开状态（255为特殊标记值）
                    userInterface->displayOutletTestGraphic(NUM_OUTLETS, 255, currentSubMode);
                }
            }
            break;
        }
        case 2: {
            /**
             * 子模式2：舵机寿命测试模式
             * 行为：所有出口同时动作，循环打开/关闭
             * 时序：
             *   - 打开状态保持时间：1秒
             *   - 关闭状态保持时间：1秒
             * 应用场景：测试所有舵机的使用寿命
             */
            interval = 1000;  // 固定1秒间隔
            testType = "lifetime test";  // 寿命测试类型
            
            // 使用单独的寿命测试逻辑
            if (currentTime - lastOutletTime >= interval) {
                lastOutletTime = currentTime;
                outletState = !outletState;
                
                // 当状态从关闭切换到打开时，增加循环计数
                if (outletState) {
                    cycleCount++;
                }
                
                // 控制所有出口同时执行相同动作
                for (int i = 0; i < NUM_OUTLETS; i++) {
                    outlets[i]->setReadyToOpen(outletState);
                    outlets[i]->execute();
                }
                
                // 更新显示内容，显示当前循环次数
                if (userInterface != nullptr) {
                    // 显示寿命测试信息，使用新的专门方法
                    userInterface->displayOutletTestGraphic(NUM_OUTLETS, cycleCount, outletState, currentSubMode);
                }
            }
            break;
        }
    }
}

/**
 * 处理出口测试的核心循环操作
 * 
 * @param currentTime 当前系统时间（毫秒），用于时间间隔计算
 * @param interval 出口状态保持时间（毫秒），根据当前状态确定保持时间
 * @param testType 测试类型描述，用于日志输出（"normally closed"或"normally open"）
 * 
 * 工作流程：
 * 1. 检查是否达到状态切换时间间隔
 * 2. 如果达到，切换出口状态（开<->关）
 * 3. 当状态切换到打开时，移动到下一个出口
 * 4. 控制当前出口执行相应动作
 * 5. 更新显示内容，指示当前测试的出口
 * 
 * 扩展说明：
 * - 所有子模式共享此核心逻辑，仅通过不同的interval和testType参数区分
 * - 添加新子模式时，只需在switch语句中添加新case，设置合适的interval和testType
 * - 无需修改此函数即可支持新的子模式行为
 */
void OutletDiagnosticHandler::processCycleOperation(unsigned long currentTime, unsigned long interval, const String& testType) {
    // 检查是否达到状态切换时间间隔
    if (currentTime - lastOutletTime >= interval) {
        // 更新时间戳
        lastOutletTime = currentTime;
        
        // 切换出口状态（开<->关）
        outletState = !outletState;
        
        // 当状态从关闭切换到打开时，移动到下一个出口
        if (outletState) {
            currentOutlet = (currentOutlet + 1) % NUM_OUTLETS;
            
            // 输出诊断信息到串口
            Serial.print("[DIAGNOSTIC] Now testing outlet " + testType + ": ");
            Serial.println(currentOutlet);
        }
        
        // 直接控制当前出口执行相应动作
        outlets[currentOutlet]->setReadyToOpen(outletState);
        outlets[currentOutlet]->execute();
        
        // 更新显示屏内容，指示当前测试状态
        if (outletState) {
            // 出口打开时，显示当前测试的出口编号
            userInterface->displayOutletTestGraphic(NUM_OUTLETS, currentOutlet, currentSubMode);
        } else {
            // 出口关闭时，显示无出口打开状态（255为特殊标记值）
            userInterface->displayOutletTestGraphic(NUM_OUTLETS, 255, currentSubMode);
        }
    }
}

void OutletDiagnosticHandler::initializeDiagnosticMode(unsigned long currentTime) {
    // 模式开始时初始化
    modeStartTime = currentTime;
    lastOutletTime = currentTime;
    
    // 根据子模式设置不同的初始outletState
    // 子模式0：常态闭合模式，初始状态为关闭
    // 子模式1：常态打开模式，初始状态为打开
    // 子模式2：寿命测试模式，初始状态为关闭
    if (currentSubMode == 1) {
        outletState = true;  // 子模式1默认打开
    } else {
        outletState = false; // 其他模式默认关闭
    }
    
    currentOutlet = 0;
    displayInitialized = false;
    cycleCount = 0;
    
    // 立即控制当前出口执行初始状态的动作
    if (currentSubMode != 2) { // 子模式2使用单独的逻辑
        outlets[currentOutlet]->setReadyToOpen(outletState);
        outlets[currentOutlet]->execute();
        
        // 更新显示内容
        if (userInterface != nullptr) {
            if (outletState) {
                userInterface->displayOutletTestGraphic(NUM_OUTLETS, currentOutlet, currentSubMode);
            } else {
                userInterface->displayOutletTestGraphic(NUM_OUTLETS, 255, currentSubMode);
            }
        }
        
        // 输出诊断信息到串口
        Serial.print("[DIAGNOSTIC] Initial outlet state " + String(currentSubMode == 0 ? "normally closed" : "normally open") + ": ");
        Serial.print(currentOutlet);
        Serial.print(" - ");
        Serial.println(outletState ? "Open" : "Closed");
    }
    
    // 打印诊断信息
    String subModeName;
    switch (currentSubMode) {
        case 0:
            subModeName = "Cycle Drop (Normally Closed)";
            break;
        case 1:
            subModeName = "Cycle Raise (Normally Open)";
            break;
        case 2:
            subModeName = "Servo Lifetime Test";
            // 只显示两行：标题和初始状态
            Serial.println("\n=== Servo Lifetime Test ===");
            Serial.println("Cycle: 0 | State: Closed");
            break;
        default:
            subModeName = "Unknown";
    }
    if (currentSubMode != 2) {
        Serial.println("[DIAGNOSTIC] Outlet Diagnostic Mode Activated - Submode: " + subModeName);
        Serial.println("[DIAGNOSTIC] Use slave button to switch submode");
    }
}
