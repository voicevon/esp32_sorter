#include "sorter.h"
#include "HardwareSerial.h"
#include "pins.h"
#include "tray_manager.h"
#include <Arduino.h>
#include <cstddef>
#include "user_interface/oled.h"

Sorter::Sorter() :
                   restartScan(false), calculateDiameter(false),
                   executeOutlets(false), resetOutlets(false),
                   reloaderOpenRequested(false), reloaderCloseRequested(false),
                   lastSpeedCheckTime(0), lastEncoderPosition(0), lastSpeed(0.0f), lastObjectCount(0) {
    // 构造函数初始化 - 使用单例模式获取实例
    encoder = Encoder::getInstance();
    simpleHmi = SimpleHMI::getInstance();
    
    // 初始化分流点索引为默认值
    for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
        outletDivergencePoints[i] = 5 + i * 5;  // 默认间距为5
    }
}

void Sorter::initialize() {
    // 初始化LED引脚为输出模式
    pinMode(STATUS_LED1_PIN, OUTPUT);
    pinMode(STATUS_LED2_PIN, OUTPUT);
    
    // 初始化LED状态为关闭
    digitalWrite(STATUS_LED1_PIN, LOW);
    digitalWrite(STATUS_LED2_PIN, LOW);
    
    // 获取直径扫描仪单例实例并初始化
    if (!scanner) {
        scanner = DiameterScanner::getInstance();
    }
    scanner->initialize();
    
    // 出口直径范围定义（单位：毫米）
    const uint8_t OUTLET_DIAMETER_RANGES[SORTER_NUM_OUTLETS][2] = {
        {0, 0},     // 出口0：特殊处理
        {20, 255},  // 出口1：直径>20mm
        {18, 20},   // 出口2：18mm<直径≤20mm
        {16, 18},   // 出口3：16mm<直径≤18mm
        {14, 16},   // 出口4：14mm<直径≤16mm
        {12, 14},   // 出口5：12mm<直径≤14mm
        {10, 12},   // 出口6：10mm<直径≤12mm
        {8, 10}     // 出口7：8mm<直径≤10mm
    };
    
    // 初始化所有出口并设置直径范围
    for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
        int minD = OUTLET_DIAMETER_RANGES[i][0];
        int maxD = OUTLET_DIAMETER_RANGES[i][1];
        outlets[i].initialize(SERVO_PINS[i], minD, maxD);
    }
    
    // 初始化出口位置
    uint8_t defaultDivergencePoints[SORTER_NUM_OUTLETS] = {1, 3, 5, 7, 9, 11, 13, 15};
    initializeDivergencePoints(defaultDivergencePoints);
    
    // 初始化托盘系统
    traySystem.resetAllTraysData();
    
    // 初始化上料器舵机
    reloaderServo.attach(RELOADER_SERVO_PIN);
    // 初始化时关闭上料器
    reloaderServo.write(SORTER_RELOADER_CLOSE_ANGLE);
    
    // 设置编码器回调，将Sorter实例和静态回调函数连接到编码器
    encoder->setPhaseCallback(this, encoderPhaseCallback);
}

// 初始化出口位置实现
void Sorter::initializeDivergencePoints(const uint8_t positions[SORTER_NUM_OUTLETS]) {
    for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
        if (positions[i] < 31) { // TOTAL_TRAYS = 31
            outletDivergencePoints[i] = positions[i];
        } else {
            // 如果提供的位置无效，使用默认值
            outletDivergencePoints[i] = 5 + i * 5;
        }
    }
}

void Sorter::onPhaseChange(int phase) {
    // 编码器中断始终工作，不受系统模式影响
    // 只执行轻量级操作，设置标志位
    scanner->sample(phase);  // 直径测量采样（受isScanning标志位控制）
    
    switch (phase) {
        // 直径扫描仪相关操作
        case 100:
            calculateDiameter = true;
            break;
        case 180:
            restartScan = true;
            break;
            
        // 出口相关操作
        case 110:
            resetOutlets = true;
            break;
        case 175:
            executeOutlets = true;
            break;
        // 上料器特殊位置控制
        case 200:
            // 上料器开启位置
            reloaderOpenRequested = true;
            break;
        case 220:
            // 上料器关闭位置
            reloaderCloseRequested = true;
            break;
        default:
            // 无需处理的phase值
            break;
    }
    
    // 检查所有出口位置
}

// 实现静态回调函数
void Sorter::encoderPhaseCallback(void* context, int phase) {
    // 将上下文转换回Sorter指针并调用成员函数
    Sorter* sorter = static_cast<Sorter*>(context);
    sorter->onPhaseChange(phase);
}

// 处理扫描仪相关任务
void Sorter::processScannerTasks() {
    if (restartScan) {
        restartScan = false;
        scanner->start();
    }
    
    if (calculateDiameter) {
        calculateDiameter = false;
        // 从传感器获取直径数据（单位为unit）
        int rawDiameterValue = scanner->getDiameterAndStop();
        // 将unit转换为毫米（diameter_mm = rawDiameterUnit / 2）
        int diameterMm = rawDiameterValue / 2;
        int objectScanCount = scanner->getTotalObjectCount();
        
        // 当直径不为0时显示原始检测数据
        if (diameterMm > 0) {
            Serial.print("原始值检测到: ");
            Serial.print(diameterMm);
            Serial.print("mm, 物体计数: ");
            Serial.println(objectScanCount);
        }
        
        // 添加新的直径数据到托盘系统（使用毫米值）
        traySystem.pushNewAsparagus(diameterMm, objectScanCount);
        prepareOutlets();
    }
}

// 处理出口相关任务
void Sorter::processOutletTasks() {
    if (executeOutlets) {
        // 执行出口动作
        executeOutlets = false;
        bool hasOutletOpened = false;
        for(int i = 0; i < SORTER_NUM_OUTLETS; i++){
            // 检查出口是否满足打开条件并实际打开
            // if (i == 0 && traySystem.getTrayScanCount(0) > 1) {
            // if (i == 0 && traySystem.getTrayScanCount(0) < 6) {
            if (false) {
                Serial.println("预设出口0打开");
                outlets[i].setReadyToOpen(true);
                hasOutletOpened = true;
            } else if (i > 0) {
                // 修改：将出口位置索引减1，因为实际托盘计数从1开始
                int adjustedPosition = outletDivergencePoints[i] - 1;
                // 确保调整后的索引在有效范围内
                if (adjustedPosition >= 0 && adjustedPosition < traySystem.getCapacity()) {
                    int diameter = traySystem.getTrayDiameter(adjustedPosition);
                    int minDiameter = outlets[i].getMatchDiameterMin();
                    int maxDiameter = outlets[i].getMatchDiameterMax();
                    
                    if ((i == 1 && diameter > minDiameter) || 
                        (i > 1 && diameter > minDiameter && diameter <= maxDiameter)) {

                        outlets[i].setReadyToOpen(true);
                        hasOutletOpened = true;
                    } else {
                        outlets[i].setReadyToOpen(false);
                    }
                } else {
                    outlets[i].setReadyToOpen(false);
                }
            } else {
                outlets[i].setReadyToOpen(false);
            }
            
            outlets[i].execute();
        }
        
        // 使用直接GPIO控制LED显示出口状态：两个LED都亮表示有出口打开
        if (hasOutletOpened) {
            digitalWrite(STATUS_LED1_PIN, HIGH);
            digitalWrite(STATUS_LED2_PIN, HIGH);
        } else {
            digitalWrite(STATUS_LED1_PIN, LOW);
            digitalWrite(STATUS_LED2_PIN, LOW);
        }
    }
    
    if (resetOutlets) {
        resetOutlets = false;
        for(int i = 0; i < SORTER_NUM_OUTLETS; i++){
            outlets[i].setReadyToOpen(false);
            outlets[i].execute();
        }
        // 复位时关闭所有LED
        digitalWrite(STATUS_LED1_PIN, LOW);
        digitalWrite(STATUS_LED2_PIN, LOW);
    }
}

// 处理上料器相关任务
void Sorter::processReloaderTasks() {
    // 处理上料器控制
    if (reloaderOpenRequested) {
        reloaderOpenRequested = false;
        // 内联实现上料器开启
        reloaderServo.write(SORTER_RELOADER_OPEN_ANGLE);
    }
    
    if (reloaderCloseRequested) {
        reloaderCloseRequested = false;
        // 内联实现上料器关闭
        reloaderServo.write(SORTER_RELOADER_CLOSE_ANGLE);
    }
}

// 出口控制公共方法实现
void Sorter::setOutletState(uint8_t outletIndex, bool open) {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex].setReadyToOpen(open);
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
    reloaderServo.write(SORTER_RELOADER_OPEN_ANGLE);
}

void Sorter::closeReloader() {
    reloaderServo.write(SORTER_RELOADER_CLOSE_ANGLE);
}

// 实现预设出口功能
void Sorter::prepareOutlets() {
    // 出口0处理
    if (traySystem.getTrayScanCount(0) > 1){
        outlets[0].setReadyToOpen(true);
    } else {
        outlets[0].setReadyToOpen(false);
    }

    // 为每个出口预设状态
    for (uint8_t i = 1; i < SORTER_NUM_OUTLETS; i++) {
        // 修改：将出口位置索引减1，因为实际托盘计数从1开始
        int outletPosition = outletDivergencePoints[i] - 1;
        int minDiameter = outlets[i].getMatchDiameterMin();
        int maxDiameter = outlets[i].getMatchDiameterMax();
        
        // 确保调整后的索引在有效范围内
        if (outletPosition >= 0 && outletPosition < traySystem.getCapacity()) {
            int diameter = traySystem.getTrayDiameter(outletPosition);
            
            // 只在直径有效并且满足出口条件时设置出口状态
            if (diameter > 0 && diameter <= 50) {
                // 检查直径范围
                if ((i == 1 && diameter > minDiameter) || (i > 1 && diameter > minDiameter && diameter <= maxDiameter)) {
                    outlets[i].setReadyToOpen(true);
                } else {
                    outlets[i].setReadyToOpen(false);
                }
            } else {
                outlets[i].setReadyToOpen(false);
            }
        } else {
            outlets[i].setReadyToOpen(false);
        }
    }
}

// 获取最新直径
int Sorter::getLatestDiameter() const {
    return traySystem.getTrayDiameter(0);
}

// 获取已经输送的托架数量
int Sorter::getTransportedTrayCount() const {
    // 每200个编码器脉冲对应一个托架移动
    const int pulsesPerTray = 200;
    int encoderPosition = encoder->getCurrentPosition();
    return encoderPosition / pulsesPerTray;
}

// 获取传送带速度（托架/秒）- 返回float类型
float Sorter::getConveyorSpeedPerSecond() {
    unsigned long currentTime = millis();
    
    // 从编码器获取当前位置
    long currentEncoderPosition = encoder->getRawCount();
    
    // 计算编码器位置变化和时间变化
    long positionDiff = currentEncoderPosition - lastEncoderPosition;
    int timeDiff = currentTime - lastSpeedCheckTime;
    
    // 每200个编码器脉冲对应一个托架移动
    const int pulsesPerTray = 200;
    
    // 设置最小时间差阈值（100ms），避免检查点刚更新后计算出异常高速值
    const int MIN_TIME_DIFF = 100;
    
    // 计算每秒的托架速度
    float speed = 0.0f;
    if (timeDiff >= MIN_TIME_DIFF) {
        // 先计算时间窗口内移动的托架数量
        float traysMoved = (float)abs(positionDiff) / pulsesPerTray;
        // 然后计算每秒的托架速度
        speed = traysMoved * 1000.0f / (float)timeDiff;
    } else if (timeDiff > 0) {
        // 时间差小于最小阈值，使用上一次的速度值
        return lastSpeed;  // 返回上一次的速度值
    }
    
    // 更新时间和位置 - 只在时间窗口结束时更新（1秒）
    if (timeDiff >= 1000) {
        lastSpeedCheckTime = currentTime;
        lastEncoderPosition = currentEncoderPosition;
    }
    
    // 更新并保存当前速度值，用于下次短时间差时使用
    if (speed > 0.0f) {
        lastSpeed = speed;
    }
    
    return speed;
}

// 获取显示数据（用于 UserInterface）
// Sorter::getDisplayData方法已移除，改用专用方法

