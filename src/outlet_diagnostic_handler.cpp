#include "outlet_diagnostic_handler.h"
#include "modular/pins.h"

OutletDiagnosticHandler::OutletDiagnosticHandler() {
}

void OutletDiagnosticHandler::initialize() {
    // 由于不同模式不会同时运行，这里不再重复初始化硬件资源
    // 出口舵机已在Sorter类的initialize方法中初始化
}

void OutletDiagnosticHandler::setOutletState(uint8_t outletIndex, bool open) {
    if (outletIndex < NUM_OUTLETS) {
        outlets[outletIndex].setReadyToOpen(open);
        outlets[outletIndex].execute();
        
        // 根据出口状态控制LED显示
        if (open) {
            digitalWrite(STATUS_LED1_PIN, HIGH);
            digitalWrite(STATUS_LED2_PIN, HIGH);
        }
    }
}

void OutletDiagnosticHandler::processTasks() {
    // 处理出口相关任务
    // 这里可以根据需要添加出口的处理逻辑
    // 目前暂时为空实现
}
