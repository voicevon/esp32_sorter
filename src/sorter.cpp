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
    // 构造函数初始化 - 使用单例模式获取实例
    encoder = Encoder::getInstance();
    hmi = SimpleHMI::getInstance();
}

void Sorter::initialize() {
    // 确保HMI实例正确获取（使用单例模式）
    if (!hmi) {
        hmi = SimpleHMI::getInstance();
    }
    
    // 初始化LED引脚为输出模式
    pinMode(STATUS_LED1_PIN, OUTPUT);
    pinMode(STATUS_LED2_PIN, OUTPUT);
    
    // 初始化LED状态为关闭
    digitalWrite(STATUS_LED1_PIN, LOW);
    digitalWrite(STATUS_LED2_PIN, LOW);
    
    // 初始化直径扫描仪
    scanner.initialize();
    
    // 初始化所有出口并设置直径范围（以毫米为单位）
    // 出口0: 扫描次数>1
    outlets[0].initialize(SERVO_PINS[0]);
    // 出口1: 直径>15mm (15mm以上)
    outlets[1].initialize(SERVO_PINS[1], 13, 255);
    // 出口2: 12mm<直径≤15mm
    outlets[2].initialize(SERVO_PINS[2], 10, 13);
    // 出口3: 10mm<直径≤12mm
    outlets[3].initialize(SERVO_PINS[3], 8, 10);
    // 出口4: 8mm≤直径≤10mm
    outlets[4].initialize(SERVO_PINS[4], 6, 8);
    
    // 初始化出口位置
    uint8_t defaultDivergencePoints[NUM_OUTLETS] = {1, 3, 5, 7, 9};
    initializeDivergencePoints(defaultDivergencePoints);
    
    // 初始化托盘系统
    traySystem.resetAllTraysData();
    
    // 初始化上料器舵机
    reloaderServo.attach(RELOADER_SERVO_PIN);
    // 初始化时关闭上料器
    reloaderServo.write(RELOADER_CLOSE_ANGLE);
    
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
    
    running = true;
}



void Sorter::onPhaseChange(int phase) {
    // 在中断中只执行轻量级操作，设置标志位
    scanner.sample(phase);  // 这个操作必须在中断中执行
    
    if (phase == 50) {
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
    // LED现在只用于显示出口状态，不再进行定期闪烁测试
    
    if (resetScannerFlag) {
        resetScannerFlag = false;
        scanner.start();
    }
    
    if (processScanDataFlag) {
        processScanDataFlag = false;
        // 从传感器获取直径数据（单位为unit）
        int rawDiameterUnit = scanner.ending_getDiameter();
        // 将unit转换为毫米（diameter_mm = rawDiameterUnit / 2）
        int diameter_mm = rawDiameterUnit / 2;
        int scanCount = scanner.getObjectCount();
        
        // 当直径不为0时显示原始检测数据
        if (diameter_mm > 0) {
            Serial.print("原始值检测到: ");
            Serial.print(diameter_mm);
            Serial.println("mm");
        }
        
        // 添加新的直径数据到托盘系统（使用毫米值）
        traySystem.addNewDiameterData(diameter_mm, scanCount);
        presetOutlets();
    }
    
    if (executeOutletsFlag) {
        // 执行出口动作
        executeOutletsFlag = false;
        bool anyOutletOpened = false;
        for(int i = 0; i < NUM_OUTLETS; i++){
            // 检查出口是否满足打开条件并实际打开
            if (i == 0 && traySystem.getTrayScanCount(0) > 1) {
                Serial.println("预设出口0打开");
                outlets[i].preOpen(true);
                anyOutletOpened = true;
            } else if (i > 0) {
                // 修改：将出口位置索引减1，因为实际托盘计数从1开始
                int adjustedPosition = divergencePointIndices[i] - 1;
                // 确保调整后的索引在有效范围内
                if (adjustedPosition >= 0 && adjustedPosition < traySystem.getTotalTrays()) {
                    int diameter = traySystem.getTrayDiameter(adjustedPosition);
                    int min = outlets[i].getMinDiameter();
                    int max = outlets[i].getMaxDiameter();
                    
                    if ((i == 1 && diameter > min) || 
                        (i > 1 && diameter > min && diameter <= max)) {

                        outlets[i].preOpen(true);
                        anyOutletOpened = true;
                    } else {
                        outlets[i].preOpen(false);
                    }
                } else {
                    outlets[i].preOpen(false);
                }
            } else {
                outlets[i].preOpen(false);
            }
            
            outlets[i].execute();
        }
        
        // 使用直接GPIO控制LED显示出口状态：两个LED都亮表示有出口打开
        if (anyOutletOpened) {
            digitalWrite(STATUS_LED1_PIN, HIGH);
            digitalWrite(STATUS_LED2_PIN, HIGH);
        } else {
            digitalWrite(STATUS_LED1_PIN, LOW);
            digitalWrite(STATUS_LED2_PIN, LOW);
        }
    }
    
    if (resetOutletsFlag) {
        resetOutletsFlag = false;
        for(int i = 0; i < NUM_OUTLETS; i++){
            outlets[i].preOpen(false);
            outlets[i].execute();
        }
        // 复位时关闭所有LED
        digitalWrite(STATUS_LED1_PIN, LOW);
        digitalWrite(STATUS_LED2_PIN, LOW);
    }
    
    // 处理上料器控制
    if (reloaderOpenFlag) {
        reloaderOpenFlag = false;
        // 内联实现上料器开启
        reloaderServo.write(RELOADER_OPEN_ANGLE);
    }
    
    if (reloaderCloseFlag) {
        reloaderCloseFlag = false;
        // 内联实现上料器关闭
        reloaderServo.write(RELOADER_CLOSE_ANGLE);
    }
}





// 出口控制公共方法实现
void Sorter::setOutletState(uint8_t outletIndex, bool open) {
    if (outletIndex < NUM_OUTLETS) {
        outlets[outletIndex].preOpen(open);
        outlets[outletIndex].execute();
        // 公共方法中使用直接GPIO控制LED显示
        if (open) {
            digitalWrite(STATUS_LED1_PIN, HIGH);
            digitalWrite(STATUS_LED2_PIN, HIGH);
        }
    }
}

// 上料器控制公共方法实现
void Sorter::openReloader() {
    reloaderServo.write(RELOADER_OPEN_ANGLE);
}

void Sorter::closeReloader() {
    reloaderServo.write(RELOADER_CLOSE_ANGLE);
}

// 实现预设出口功能
void Sorter::presetOutlets() {
    // 出口0处理
    if (traySystem.getTrayScanCount(0) > 1){
        outlets[0].preOpen(true);
    } else {
        outlets[0].preOpen(false);
    }

    // 为每个出口预设状态
    for (uint8_t i = 1; i < NUM_OUTLETS; i++) {
        // 修改：将出口位置索引减1，因为实际托盘计数从1开始
        int outletPosition = divergencePointIndices[i] - 1;
        int min = outlets[i].getMinDiameter();
        int max = outlets[i].getMaxDiameter();
        
        // 确保调整后的索引在有效范围内
        if (outletPosition >= 0 && outletPosition < traySystem.getTotalTrays()) {
            int diameter = traySystem.getTrayDiameter(outletPosition);
            
            // 只在直径有效并且满足出口条件时设置出口状态
            if (diameter > 0 && diameter <= 50) {
                // 检查直径范围
                if ((i == 1 && diameter > min) || (i > 1 && diameter > min && diameter <= max)) {
                    outlets[i].preOpen(true);
                } else {
                    outlets[i].preOpen(false);
                }
            } else {
                outlets[i].preOpen(false);
            }
        } else {
            outlets[i].preOpen(false);
        }
    }
}

