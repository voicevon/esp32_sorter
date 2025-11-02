#include "sorter.h"
#include "pins.h"
#include <Arduino.h>

Sorter::Sorter() : 
    isRunning(false),
    isInitialized(false) {
}

Sorter::~Sorter() {
}

void Sorter::initialize() {
    // 初始化编码器
    encoder.initialize();
    
    // 设置默认出口位置
    uint8_t defaultOutlets[NUM_OUTLETS] = {10, 15, 20, 25, 30};
    initializeDivergencePoints(defaultOutlets);
    
    // 初始化出口
    outlet1.initialize(SERVO_PINS[0]);
    outlet2.initialize(SERVO_PINS[1]);
    outlet3.initialize(SERVO_PINS[2]);
    outlet4.initialize(SERVO_PINS[3]);
    outlet5.initialize(SERVO_PINS[4]);
    
    isRunning = false;
}

void Sorter::spin_Once() {
    if (!isRunning) {
        return;
    }
    
    // 获取当前编码器位置
    int currentPosition = encoder.getCurrentPosition();
    
    // 处理位置变化
    handlePositionChange(currentPosition);
    
    // 控制出口动作
    controlOutlets(currentPosition);
}

void Sorter::start() {
    isRunning = true;
}

void Sorter::stop() {
    isRunning = false;
    
    // 停止时复位所有出口
    outlet1.close();
    outlet2.close();
    outlet3.close();
    outlet4.close();
    outlet5.close();
}

void Sorter::reset() {
    stop();
    
    // 重置编码器
    // 编码器重置 - 注意：reset()是私有方法，不能直接调用
    
    // 重置所有直径数据
    resetAllCarriagesData();
}

void Sorter::receiveDiameterData(float diameter) {
    // 将直径数据添加到传送带系统
    addNewDiameterData(diameter);
}

void Sorter::handlePositionChange(int position) {
    // 移动所有托架数据
    moveCarriagesData();
}

void Sorter::controlOutlets(int position) {
    // 检查是否需要激活出口
    uint8_t outletToActivate = checkAndExecuteAssignment((uint8_t)position);
    
    if (outletToActivate > 0 && outletToActivate <= 5) {
        // 打开对应的出口
        switch (outletToActivate) {
            case 1:
                outlet1.open();
                break;
            case 2:
                outlet2.open();
                break;
            case 3:
                outlet3.open();
                break;
            case 4:
                outlet4.open();
                break;
            case 5:
                outlet5.open();
                break;
        }
        
        // 短暂延迟后关闭出口（复位）
        delay(100);
        switch (outletToActivate) {
            case 1:
                outlet1.close();
                break;
            case 2:
                outlet2.close();
                break;
            case 3:
                outlet3.close();
                break;
            case 4:
                outlet4.close();
                break;
            case 5:
                outlet5.close();
                break;
        }
    }
}