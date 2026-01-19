#include "outlet_diagnostic_handler.h"
#include "modular/pins.h"

OutletDiagnosticHandler::OutletDiagnosticHandler() {
    // 初始化诊断模式状态变量
    modeStartTime = 0;
    lastOutletTime = 0;
    outletState = false;
    currentOutlet = 0;
    displayInitialized = false;
    currentSubMode = 0;
    userInterface = nullptr;
    
    // 初始化outlets数组为nullptr
    for (int i = 0; i < OUTLET_COUNT; i++) {
        outlets[i] = nullptr;
    }
}

void OutletDiagnosticHandler::initialize() {
    // 由于不同模式不会同时运行，这里不再重复初始化硬件资源
    // 出口舵机已在Sorter类的initialize方法中初始化
}

void OutletDiagnosticHandler::setUserInterface(UserInterface* ui) {
    userInterface = ui;
}

void OutletDiagnosticHandler::setOutlet(uint8_t index, Outlet* outlet) {
    if (index < NUM_OUTLETS) {
        outlets[index] = outlet;
    }
}

void OutletDiagnosticHandler::switchToNextSubMode() {
    // 切换子模式（0和1之间切换）
    currentSubMode = (currentSubMode + 1) % 2;
    
    // 打印子模式切换信息
    String subModeName = currentSubMode == 0 ? "Cycle Drop (Normally Open)" : "Cycle Raise (Normally Closed)";
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
    unsigned long interval;
    String testType;
    
    // 根据子模式执行相对独立的逻辑
    switch (currentSubMode) {
        case 0:
            // 轮巡降落（常态打开，偶尔闭合）
            // 打开保持时间：4.5秒，关闭保持时间：1.5秒
            interval = outletState ? 4500 : 1500;
            testType = "normally closed";
            processCycleOperation(currentTime, interval, testType);
            break;
        case 1:
            // 轮巡上升（常态闭合，偶尔打开）
            // 关闭保持时间：4.5秒，打开保持时间：1.5秒
            interval = outletState ? 1500 : 4500;
            testType = "normally open";
            processCycleOperation(currentTime, interval, testType);
            break;
    }
}

void OutletDiagnosticHandler::processCycleOperation(unsigned long currentTime, unsigned long interval, const String& testType) {
    if (currentTime - lastOutletTime >= interval) {
        lastOutletTime = currentTime;
        outletState = !outletState;
        
        // 当状态从关闭切换到打开时，移动到下一个出口
        if (outletState) {
            currentOutlet = (currentOutlet + 1) % NUM_OUTLETS;
            Serial.print("[DIAGNOSTIC] Now testing outlet " + testType + ": ");
            Serial.println(currentOutlet);
        }
        
        // 直接控制当前出口状态
        outlets[currentOutlet]->setReadyToOpen(outletState);
        outlets[currentOutlet]->execute();
        
        // 更新显示内容
        // 直接使用类成员变量，避免不必要的函数调用
        if (outletState) {
            userInterface->displayOutletTestGraphic(NUM_OUTLETS, currentOutlet, currentSubMode);
        } else {
            userInterface->displayOutletTestGraphic(NUM_OUTLETS, 255, currentSubMode);  // 255表示没有打开的出口
        }
    }
}

void OutletDiagnosticHandler::initializeDiagnosticMode(unsigned long currentTime) {
    // 模式开始时初始化
    modeStartTime = currentTime;
    lastOutletTime = currentTime;
    outletState = false;
    currentOutlet = 0;
    displayInitialized = false;
    
    // 打印诊断信息
    Serial.println("[DIAGNOSTIC] Outlet Diagnostic Mode Activated - Submode: " + String(currentSubMode == 0 ? "Cycle Drop (Normally Open)" : "Cycle Raise (Normally Closed)"));
    Serial.println("[DIAGNOSTIC] Use slave button to switch submode");
}
