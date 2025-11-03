#include "sorter.h"
#include "pins.h"
#include "tray_system.h"
#include <Arduino.h>
#include <cstddef>



Sorter::Sorter() {
    // 获取编码器单例实例
    encoder = Encoder::getInstance();
    running = false;
    

}

void Sorter::initialize() {
    // 初始化直径扫描仪
    scanner.initialize();
    
    // 初始化所有出口
    for (int i = 0; i < NUM_OUTLETS; i++) {
        outlets[i].initialize(SERVO_PINS[i]);
    }
    
    // 初始化出口位置
    uint8_t defaultDivergencePoints[NUM_OUTLETS] = {5, 10, 15, 20, 25};
    initializeDivergencePoints(defaultDivergencePoints);
    
    // 初始化托盘系统
    traySystem.resetAllTraysData();
    
    // 设置编码器回调，将Sorter实例和静态回调函数连接到编码器
    encoder->setPhaseCallback(this, staticPhaseCallback);
}

// 初始化出口位置实现
void Sorter::initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]) {
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        if (positions[i] < 31) { // TOTAL_TRAYS = 31
            divergencePointIndices[i] = positions[i];
        } else {
            // 如果提供的位置无效，使用默认值
            divergencePointIndices[i] = 5 + i * 5;
        }
    }
    
    Serial.println("出口位置已初始化");

    running = true;
}



void Sorter::onPhaseChange(int phase) {
    // 根据编码器相位变化处理托盘数据
    scanner.sample(phase);
    
    // 检查是否到达扫描位置
    if (phase == 160) {
        // 进入扫描范围，重置扫描仪
        scanner.reset();
    }
    else if (phase == 50) { // 扫描范围结束
        // 从传感器获取直径数据
        int diameter = scanner.getDiameter();
        int scanCount = scanner.getObjectCount();
        
        // 添加新的直径数据到托盘系统
        traySystem.addNewDiameterData(diameter, scanCount);
        PresetOutlets();
    } else if (phase == 120) {
        
        for(int i = 0; i < NUM_OUTLETS; i++){
            outlets[i].execute();
        }

    } else if (phase == 3) {
        // 最后一个相位：出口复位（可添加复位逻辑）
        for(int i = 0; i < NUM_OUTLETS; i++){
            outlets[i].PreOpen(false);
            outlets[i].execute();
        }
    }
    
    // 检查所有出口位置
}

// 实现静态回调函数
void Sorter::staticPhaseCallback(void* context, int phase) {
    // 将上下文转换回Sorter指针并调用成员函数
    Sorter* sorter = static_cast<Sorter*>(context);
    sorter->onPhaseChange(phase);
}





void Sorter::PresetOutlets() {
    if (traySystem.getTrayScanCount(0) > 1){
        outlets[0].PreOpen(true);
    }

    for (uint8_t i = 1; i < NUM_OUTLETS; i++) {
        int outletPosition = divergencePointIndices[i];
        int min = outlets[i].getMinDiameter();
        int max = outlets[i].getMaxDiameter();
        if (traySystem.getTrayDiameter(outletPosition) > min && traySystem.getTrayDiameter(outletPosition) < max) {
            outlets[i].PreOpen(true);
        }
    }
}