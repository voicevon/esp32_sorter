#include "outlet_diagnostic_handler.h"
#include "config.h"

OutletDiagnosticHandler::OutletDiagnosticHandler() : 
    modeStartTime(0), 
    lastOutletTime(0), 
    outletState(false), 
    currentOutlet(0), 
    displayInitialized(false), 
    currentSubMode(0),
    lastSubMode(-1),
    lastUpdateTime(0),
    cycleCount(0), 
    userInterface(nullptr) {
    for (int i = 0; i < NUM_OUTLETS; i++) {
        outlets[i] = nullptr;
    }
}

void OutletDiagnosticHandler::begin() {
    Serial.println("[DIAGNOSTIC] Outlet Diagnostic Started");
    modeStartTime = 0; // Trigger initialization in update()
}

void OutletDiagnosticHandler::end() {
    Serial.println("[DIAGNOSTIC] Outlet Diagnostic Ended");
    // Ensure all outlets are closed on exit
    for (int i = 0; i < NUM_OUTLETS; i++) {
        if (outlets[i]) {
            outlets[i]->setReadyToOpen(false);
            outlets[i]->execute();
        }
    }
}

void OutletDiagnosticHandler::initialize(UserInterface* ui) {
    // 由于不同模式不会同时运行，这里不再重复初始化硬件资源
    // 出口电磁铁已在Sorter类的initialize方法中初始化
    
    // 设置UserInterface实例，用于显示诊断信息
    userInterface = ui;
}

void OutletDiagnosticHandler::setOutlet(uint8_t index, Outlet* outlet) {
    if (index < NUM_OUTLETS) {
        outlets[index] = outlet;
    }
}

void OutletDiagnosticHandler::setSubMode(int mode) {
    currentSubMode = mode;
    lastSubMode = -1; // 强制触发一次显示刷新
    cycleCount = 0;
    outletState = false;
    
    String subModeName = "";
    switch(mode) {
        case 0: subModeName = "Normally Closed Test"; break;
        case 1: subModeName = "Single Outlet Test"; break;
        case 2: subModeName = "Lifetime Cycle Test"; break;
        default: subModeName = "Unknown Mode"; break;
    }
    Serial.println("[DIAGNOSTIC] Outlet Mode explicitly set to: " + subModeName);
}

void OutletDiagnosticHandler::update(uint32_t currentMs, bool btnPressed) {
    if (btnPressed) {
        handleReturnToMenu();
        return;
    }
    
    // 模式开始时初始化
    if (modeStartTime == 0 || currentSubMode != lastSubMode) { // Re-initialize if submode changes
        initializeDiagnosticMode(currentMs);
        lastSubMode = currentSubMode;
    }
    
    // 声明局部变量
    uint32_t interval;    // 出口状态保持时间间隔（毫秒）
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
             *   - 关闭状态保持时间：0.5秒（outletState为false时）
             *   - 打开状态保持时间：1.0秒（outletState为true时）
             * 应用场景：测试弹簧降落式出口的正常工作
             */
            interval = outletState ? 1000 : 500;
            testType = "normally closed";  // 表示出口在非测试状态下是常闭的
            processCycleOperation(currentMs, interval, testType);
            break;
        }
        case 1: {
            /**
             * 子模式1：单点测试模式 (Single Outlet Test)
             * 行为：用户选择特定出口，周期性打开/关闭
             * 时序：
             *   - 打开状态保持时间：0.3秒
             *   - 关闭状态保持时间：0.5秒
             */
            interval = outletState ? 300 : 500;
            if (currentMs - lastOutletTime >= interval) {
                lastOutletTime = currentMs;
                outletState = !outletState; // 切换状态
                
                // 确保只有当前选中的出口动作
                for (int i = 0; i < NUM_OUTLETS; i++) {
                    if (i == currentOutlet) {
                        outlets[i]->setReadyToOpen(outletState);
                    } else {
                        outlets[i]->setReadyToOpen(false); // 其他强制关闭
                    }
                    outlets[i]->execute();
                }
                
                userInterface->displayOutletTestGraphic(NUM_OUTLETS, currentOutlet, outletState, currentSubMode);
            }
            break;
        }
        case 2: {
            /**
             * 子模式2：电磁铁寿命测试模式
             * 行为：轮流打开关闭每个出口 (1开->1关->2开->2关...)
             * 时序：
             *   - 打开状态保持时间：0.3秒
             *   - 关闭状态保持时间：0.1秒
             * 计数：每 8 次开合动作显示更新一次
             */
            interval = outletState ? 300 : 100;  // open 0.3s / close 0.1s
            testType = "lifetime test";  
            
            if (currentMs - lastOutletTime >= interval) {
                lastOutletTime = currentMs;
                outletState = !outletState;
                
                if (outletState) {
                    cycleCount++;
                    if (cycleCount % 8 == 0) {  // 每8次更新一次显示
                        userInterface->displayOutletLifetimeGraphic(NUM_OUTLETS, cycleCount, outletState, currentSubMode);
                    }
                } else {
                    // 当关闭时，准备切换到下一个出口
                    currentOutlet = (currentOutlet + 1) % NUM_OUTLETS;
                }
                
                // 动作生成，只操作当前的出口
                for (int i = 0; i < NUM_OUTLETS; i++) {
                    if (i == currentOutlet) {
                        outlets[i]->setReadyToOpen(outletState);
                    } else {
                        outlets[i]->setReadyToOpen(false);
                    }
                    outlets[i]->execute();
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
void OutletDiagnosticHandler::processCycleOperation(uint32_t currentTime, uint32_t interval, const String& testType) {
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
        userInterface->displayOutletTestGraphic(NUM_OUTLETS, currentOutlet, outletState, currentSubMode);
    }
}

void OutletDiagnosticHandler::initializeDiagnosticMode(uint32_t currentTime) {
    // 模式开始时初始化
    modeStartTime = currentTime;
    lastOutletTime = currentTime;
    
    // 根据子模式设置不同的初始outletState
    // 子模式0：常态闭合模式，初始状态为关闭
    // 子模式1：寿命测试模式，初始状态为关闭
    outletState = false; // 所有模式默认关闭
    
    currentOutlet = 0;
    displayInitialized = false;
    cycleCount = 0;
    
    // 立即控制当前出口执行初始状态的动作
    if (currentSubMode != 2) { // 子模式2使用单独的逻辑
        outlets[currentOutlet]->setReadyToOpen(outletState);
        outlets[currentOutlet]->execute();
        
        // 更新显示内容
        userInterface->displayOutletTestGraphic(NUM_OUTLETS, currentOutlet, outletState, currentSubMode);
        
        // 输出诊断信息到串口
        Serial.print("[DIAGNOSTIC] Initial outlet state normally closed: ");
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
            subModeName = "Single Outlet Test";
            break;
        case 2:
            subModeName = "Solenoid Lifetime Test";
            // 只显示两行：标题和初始状态
            Serial.println("\n=== Solenoid Lifetime Test ===");
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

// 增加处理编码器输入的函数
void OutletDiagnosticHandler::handleEncoderInput(int delta) {
    if (currentSubMode == 1) { // 仅在 Single Outlet Test 模式下生效
        if (delta != 0) {
            // 改变当前选中的出口，处理向下溢出和向上溢出的安全包裹
            int newOutlet = (int)currentOutlet + delta;
            while (newOutlet < 0) {
                newOutlet += NUM_OUTLETS;
            }
            currentOutlet = (uint8_t)(newOutlet % NUM_OUTLETS);
            
            // 立即重置状态，关闭所有并重新开始测试选中的出口
            outletState = false;
            lastOutletTime = millis(); // 重置计时器
            
            for (int i = 0; i < NUM_OUTLETS; i++) {
                outlets[i]->setReadyToOpen(false);
                outlets[i]->execute();
            }
            
            userInterface->displayOutletTestGraphic(NUM_OUTLETS, currentOutlet, false, currentSubMode);
            
            Serial.print("[DIAGNOSTIC] Selected outlet changed to: ");
            Serial.println(currentOutlet);
        }
    }
}
