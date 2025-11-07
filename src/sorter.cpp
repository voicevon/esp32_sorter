#include "sorter.h"
#include "HardwareSerial.h"
#include "pins.h"
#include "tray_system.h"
#include <Arduino.h>
#include <cstddef>



Sorter::Sorter() : running(false),
                   resetScannerFlag(false), processScanDataFlag(false),
                   executeOutletsFlag(false), resetOutletsFlag(false),
                   reloaderOpenFlag(false), reloaderCloseFlag(false) {
    // 构造函数初始化
    encoder = Encoder::getInstance();
}

void Sorter::initialize() {
    // 初始化直径扫描仪
    scanner.initialize();
    
    // 初始化所有出口并设置直径范围
    // 出口0: 扫描次数>1
    outlets[0].initialize(SERVO_PINS[0]);
    // 出口1: 直径>12mm
    outlets[1].initialize(SERVO_PINS[1], 15, 255);
    // 出口2: 9mm<直径≤12mm
    outlets[2].initialize(SERVO_PINS[2], 12, 15);
    // 出口3: 6mm<直径≤9mm
    outlets[3].initialize(SERVO_PINS[3], 10, 12);
    // 出口4: 直径≤6mm
    outlets[4].initialize(SERVO_PINS[4], 0, 10);
    
    // 初始化出口位置
    uint8_t defaultDivergencePoints[NUM_OUTLETS] = {1, 3, 5, 7, 9};
    initializeDivergencePoints(defaultDivergencePoints);
    
    // 初始化托盘系统
    traySystem.resetAllTraysData();
    
    // 初始化上料器舵机
    reloaderServo.attach(RELOADER_SERVO_PIN);
    // 初始化时关闭上料器
    reloaderServo.write(RELOADER_CLOSE_ANGLE);
    Serial.println("上料器已初始化并关闭");
    
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
    // Serial.println("onPhaseChange: " + String(phase));
    // 在中断中只执行轻量级操作，设置标志位
    scanner.sample(phase);  // 这个操作必须在中断中执行
    
    if (phase == 50) {
        // Serial.println("[SORTER] Reset Scanner");
        resetScannerFlag = true;
    } else if (phase == 160) {
        processScanDataFlag = true;
    } else if (phase == 80) {
        executeOutletsFlag = true;
    } else if (phase == 195) {
        resetOutletsFlag = true;
    } 
    // 上料器特殊位置控制
    else if (phase == 200) {
        // 上料器开启位置
        reloaderOpenFlag = true;
    } else if (phase == 220) {
        // 上料器关闭位置
        reloaderCloseFlag = true;
    }
    
    // 检查所有出口位置
}

// 实现静态回调函数
void Sorter::staticPhaseCallback(void* context, int phase) {
    // 将上下文转换回Sorter指针并调用成员函数
    Sorter* sorter = static_cast<Sorter*>(context);
    sorter->onPhaseChange(phase);
}

void Sorter::spinOnce() {
    // 在主循环中执行耗时操作
    if (resetScannerFlag) {
        resetScannerFlag = false;
        scanner.start();
    }
    
    if (processScanDataFlag) {
        // Serial.println("[SORTER] Process Scan Data");
        processScanDataFlag = false;
        // 从传感器获取直径数据
        int diameter = scanner.ending_getDiameter();
        int scanCount = scanner.getObjectCount();
        
        // 添加新的直径数据到托盘系统
        traySystem.addNewDiameterData(diameter, scanCount);
        presetOutlets();
    }
    
    if (executeOutletsFlag) {
        // Serial.println("[SORTER] Execute Outlets");
        executeOutletsFlag = false;
        for(int i = 0; i < NUM_OUTLETS; i++){
            outlets[i].execute();
        }
    }
    
    if (resetOutletsFlag) {
        // Serial.println("[SORTER] Reset Outlets");
        resetOutletsFlag = false;
        for(int i = 0; i < NUM_OUTLETS; i++){
            outlets[i].preOpen(false);
            outlets[i].execute();
        }
    }
    
    // 处理上料器控制
    if (reloaderOpenFlag) {
        reloaderOpenFlag = false;
        // 内联实现上料器开启
        reloaderServo.write(RELOADER_OPEN_ANGLE);
        Serial.println("上料器已开启");
    }
    
    if (reloaderCloseFlag) {
        reloaderCloseFlag = false;
        // 内联实现上料器关闭
        reloaderServo.write(RELOADER_CLOSE_ANGLE);
        Serial.println("上料器已关闭");
    }
}






// 出口控制公共方法实现
void Sorter::setOutletState(uint8_t outletIndex, bool open) {
    if (outletIndex < NUM_OUTLETS) {
        outlets[outletIndex].preOpen(open);
        outlets[outletIndex].execute();
        Serial.print("[SORTER] 出口 ");
        Serial.print(outletIndex);
        Serial.println(open ? " 已打开" : " 已关闭");
    }
}

// 上料器控制公共方法实现
void Sorter::openReloader() {
    reloaderServo.write(RELOADER_OPEN_ANGLE);
    Serial.println("上料器已开启（通过公共方法）");
}

void Sorter::closeReloader() {
    reloaderServo.write(RELOADER_CLOSE_ANGLE);
    Serial.println("上料器已关闭（通过公共方法）");
}

// 实现预设出口功能
void Sorter::presetOutlets() {
    if (traySystem.getTrayScanCount(0) > 1){
        outlets[0].preOpen(true);
    }

    for (uint8_t i = 1; i < NUM_OUTLETS; i++) {
        int outletPosition = divergencePointIndices[i];
        int min = outlets[i].getMinDiameter();
        int max = outlets[i].getMaxDiameter();
        int diameter = traySystem.getTrayDiameter(outletPosition);
        
        // 跳过无效直径值(0)
        if (diameter == 0) continue;
        
        // 特殊处理出口1: 直径>12mm
        if (i == 1 && diameter > min) {
            outlets[i].preOpen(true);
        }
        // 其他出口: 直径在[min+1, max]范围内
        else if (diameter > min && diameter <= max) {
            outlets[i].preOpen(true);
        }
    }
}