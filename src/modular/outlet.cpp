#include "outlet.h"

// 构造函数已在头文件中通过成员初始化列表处理

/**
 * 初始化出口逻辑状态
 */
void Outlet::initialize() {
    // 逻辑初始化：强制设定为关闭位置
    executeClose();
}

/**
 * 更新脉冲时序（由 Sorter 定时调用）
 */
void Outlet::update() {
    if (isPulsing) {
        if (millis() - pulseStateChangeTime >= PULSE_DURATION) {
            stopPulse();
        }
    }
}

/**
 * 执行目标动作（当 readyToOpenState 改变时触发）
 */
void Outlet::execute() {
    // 如果目标位置与当前物理留驻位置一致且没有正在发出的脉冲，则忽略
    if (readyToOpenState == physicalOpen && !isPulsing) {
        return;
    }

    // 如果已经在向目标方向发送脉冲，则维持当前状态
    if (isPulsing && targetPulseState == readyToOpenState) {
        return; 
    }

    // 根据逻辑目标启动 H 桥换向脉冲
    if (readyToOpenState) {
        executeOpen();
    } else {
        executeClose();
    }
}

void Outlet::executeOpen() {
    isPulsing = true;
    pulseStateChangeTime = millis();
    targetPulseState = true;
    physicalOpen = true; 
}

void Outlet::executeClose() {
    isPulsing = true;
    pulseStateChangeTime = millis();
    targetPulseState = false; 
    physicalOpen = false;
}

void Outlet::stopPulse() {
    isPulsing = false;
}
