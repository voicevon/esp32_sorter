#include "sorter.h"
#include "HardwareSerial.h"
#include "pins.h"
#include "tray_system.h"
#include <Arduino.h>
#include <cstddef>
#include "oled.h"



Sorter::Sorter() : running(false),
                   resetScannerFlag(false), processScanDataFlag(false),
                   executeOutletsFlag(false), resetOutletsFlag(false),
                   reloaderOpenFlag(false), reloaderCloseFlag(false),
                   lastSpeedCheckTime(0), lastObjectCount(0) {
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
    outlets[1].initialize(SERVO_PINS[1], 20,255 );
    // 出口2: 12mm<直径≤15mm
    outlets[2].initialize(SERVO_PINS[2], 18, 20);
    // 出口3: 10mm<直径≤12mm
    outlets[3].initialize(SERVO_PINS[3], 16, 18);
    // 出口4: 8mm≤直径≤10mm
    outlets[4].initialize(SERVO_PINS[4], 14, 16);
    // 出口5: 6mm<直径≤8mm
    outlets[5].initialize(SERVO_PINS[5], 12, 14);
    // 出口6: 4mm<直径≤6mm
    outlets[6].initialize(SERVO_PINS[6], 10, 12);
    // 出口7: 直径≤4mm (4mm以下)
    outlets[7].initialize(SERVO_PINS[7], 8, 10);
    
    // 初始化出口位置
    uint8_t defaultDivergencePoints[NUM_OUTLETS] = {1, 3, 5, 7, 9, 11, 13, 15};
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
    
    if (phase == 180) {
        resetScannerFlag = true;
    } else if (phase == 100) {
        processScanDataFlag = true;
    } else if (phase == 175) {
        executeOutletsFlag = true;
    } else if (phase == 110) {
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
        int rawDiameterUnit = scanner.getDiameterAndStop();
        // 将unit转换为毫米（diameter_mm = rawDiameterUnit / 2）
        int diameter_mm = rawDiameterUnit / 2;
        int scanCount = scanner.getObjectCount();
        
        // 当直径不为0时显示原始检测数据
        if (diameter_mm > 0) {
            Serial.print("原始值检测到: ");
            Serial.print(diameter_mm);
            Serial.print("mm, 物体计数: ");
            Serial.println(scanCount);
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
            // if (i == 0 && traySystem.getTrayScanCount(0) > 1) {
            // if (i == 0 && traySystem.getTrayScanCount(0) < 6) {
            if (false) {
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

// 获取最新直径
int Sorter::getLatestDiameter() const {
    return traySystem.getTrayDiameter(0);
}

// 获取托架数量
int Sorter::getTrayCount() const {
    // 假设每40个编码器脉冲对应一个托架移动
    const int pulsesPerTray = 40;
    int encoderPosition = encoder->getCurrentPosition();
    return encoderPosition / pulsesPerTray;
}

// 获取分拣速度
int Sorter::getSortingSpeed() {
    unsigned long currentTime = millis();
    int currentCount = scanner.getObjectCount();
    int countDiff = currentCount - lastObjectCount;
    int timeDiff = currentTime - lastSpeedCheckTime;
    
    // 计算每小时的速度
    int speed = 0;
    if (timeDiff > 0) {
        speed = (countDiff * 3600000) / timeDiff;
    }
    
    // 更新时间和计数
    if (timeDiff > 1000) { // 每秒更新一次
        lastSpeedCheckTime = currentTime;
        lastObjectCount = currentCount;
    }
    
    return speed;
}

// 获取分拣速度（根/秒）
int Sorter::getSortingSpeedPerSecond() {
    unsigned long currentTime = millis();
    int currentCount = scanner.getObjectCount();
    int countDiff = currentCount - lastObjectCount;
    int timeDiff = currentTime - lastSpeedCheckTime;
    
    // 计算每秒的速度
    int speed = 0;
    if (timeDiff > 0) {
        speed = (countDiff * 1000) / timeDiff;
    }
    
    return speed;
}

// 获取分拣速度（根/分钟）
int Sorter::getSortingSpeedPerMinute() {
    unsigned long currentTime = millis();
    int currentCount = scanner.getObjectCount();
    int countDiff = currentCount - lastObjectCount;
    int timeDiff = currentTime - lastSpeedCheckTime;
    
    // 计算每分钟的速度
    int speed = 0;
    if (timeDiff > 0) {
        speed = (countDiff * 60000) / timeDiff;
    }
    
    return speed;
}

void Sorter::displayIOStatus() {
    scanner.displayIOStatus();
}

void Sorter::displayRawDiameters() {
    scanner.displayRawDiameters();
}

// 获取显示数据（用于 UserInterface）
DisplayData Sorter::getDisplayData(SystemMode currentMode, int normalSubMode, int encoderSubMode, int outletSubMode) {
    DisplayData data;
    
    // 设置系统模式信息
    data.currentMode = currentMode;
    data.outletCount = NUM_OUTLETS;
    
    // 设置子模式信息
    data.normalSubMode = normalSubMode;
    data.encoderSubMode = encoderSubMode;
    data.outletSubMode = outletSubMode;
    
    // 设置编码器信息
    data.encoderPosition = encoder->getCurrentPosition();
    data.encoderPositionChanged = encoder->hasPositionChanged();
    
    // 设置分拣速度信息
    data.sortingSpeedPerSecond = getSortingSpeedPerSecond();
    data.sortingSpeedPerMinute = getSortingSpeedPerMinute();
    data.sortingSpeedPerHour = getSortingSpeed();
    
    // 设置统计信息
    data.identifiedCount = scanner.getObjectCount();
    data.trayCount = getTrayCount();
    
    // 设置直径信息
    data.latestDiameter = getLatestDiameter();
    
    // 设置出口测试模式信息（默认没有打开的出口）
    data.openOutlet = 255;
    
    return data;
}

