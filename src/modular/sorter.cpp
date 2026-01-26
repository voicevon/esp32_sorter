#include "sorter.h"
#include "HardwareSerial.h"
#include "pins.h"
#include "tray_manager.h"
#include <Arduino.h>
#include <cstddef>
#include <EEPROM.h>

Sorter::Sorter() :
                   shouldRestartScan(false), shouldCalculateDiameter(false),
                   executeOutlets(false), resetOutlets(false),
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
    // 获取直径扫描仪单例实例并初始化
    if (!scanner) {
        scanner = DiameterScanner::getInstance();
    }
    scanner->initialize();
    
    // 初始化EEPROM
    EEPROM.begin(512);
    
    // 定义EEPROM中存储直径范围的起始地址
    const int EEPROM_DIAMETER_RANGES_ADDR = 0;
    // 定义EEPROM中存储舵机位置的起始地址
    const int EEPROM_SERVO_POSITIONS_ADDR = 0x12;
    const int EEPROM_SERVO_MAGIC_ADDR = 0x32;
    
    // 出口直径范围数组
    uint8_t outletDiameterRanges[SORTER_NUM_OUTLETS][2];
    // 出口舵机位置数组
    uint8_t outletServoClosedPositions[SORTER_NUM_OUTLETS];
    uint8_t outletServoOpenPositions[SORTER_NUM_OUTLETS];
    
    // 从EEPROM读取直径范围数据
    bool useDefaultValues = false;
    
    // 检查EEPROM是否有有效数据（通过检查第一个字节是否为0xAA）
    uint8_t magicByte = EEPROM.read(EEPROM_DIAMETER_RANGES_ADDR);
    if (magicByte != 0xAA) {
        // EEPROM中没有有效数据，使用默认值
        useDefaultValues = true;
        Serial.println("[SORTER] No valid diameter ranges in EEPROM, using default values");
    }
    
    if (useDefaultValues) {
        // 使用默认直径范围
        outletDiameterRanges[0][0] = 0;     outletDiameterRanges[0][1] = 0;     // 出口0：特殊处理
        outletDiameterRanges[1][0] = 20;    outletDiameterRanges[1][1] = 255;  // 出口1：直径>20mm
        outletDiameterRanges[2][0] = 18;    outletDiameterRanges[2][1] = 20;   // 出口2：18mm<直径≤20mm
        outletDiameterRanges[3][0] = 16;    outletDiameterRanges[3][1] = 18;   // 出口3：16mm<直径≤18mm
        outletDiameterRanges[4][0] = 14;    outletDiameterRanges[4][1] = 16;   // 出口4：14mm<直径≤16mm
        outletDiameterRanges[5][0] = 12;    outletDiameterRanges[5][1] = 14;   // 出口5：12mm<直径≤14mm
        outletDiameterRanges[6][0] = 10;    outletDiameterRanges[6][1] = 12;   // 出口6：10mm<直径≤12mm
        outletDiameterRanges[7][0] = 8;     outletDiameterRanges[7][1] = 10;   // 出口7：8mm<直径≤10mm
        
        // 将默认值写入EEPROM
        EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR, 0xAA); // 写入魔术字节
        for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
            EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2, outletDiameterRanges[i][0]);
            EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2 + 1, outletDiameterRanges[i][1]);
        }
        EEPROM.commit();
        Serial.println("[SORTER] Default diameter ranges written to EEPROM");
    } else {
        // 从EEPROM读取直径范围
        for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
            outletDiameterRanges[i][0] = EEPROM.read(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2);
            outletDiameterRanges[i][1] = EEPROM.read(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2 + 1);
        }
        Serial.println("[SORTER] Diameter ranges loaded from EEPROM");
    }
    
    // 打印加载的直径范围
    Serial.println("[SORTER] Loaded diameter ranges:");
    for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
        Serial.print("Outlet ");
        Serial.print(i);
        Serial.print(": ");
        Serial.print(outletDiameterRanges[i][0]);
        Serial.print(" - ");
        Serial.println(outletDiameterRanges[i][1]);
    }
    
    // 从EEPROM读取舵机位置数据
    bool useDefaultServoPositions = false;
    uint8_t servoMagicByte = EEPROM.read(EEPROM_SERVO_MAGIC_ADDR);
    if (servoMagicByte != 0xBB) {
        // EEPROM中没有有效舵机位置数据，使用默认值
        useDefaultServoPositions = true;
        Serial.println("[SORTER] No valid servo positions in EEPROM, using default values");
    }
    
    if (useDefaultServoPositions) {
        // 使用默认舵机位置
        for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
            outletServoClosedPositions[i] = SERVO_CLOSED_POSITION;
            outletServoOpenPositions[i] = SERVO_OPEN_POSITION;
        }
    } else {
        // 从EEPROM读取舵机位置
        for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
            outletServoClosedPositions[i] = EEPROM.read(EEPROM_SERVO_POSITIONS_ADDR + i);
            outletServoOpenPositions[i] = EEPROM.read(EEPROM_SERVO_POSITIONS_ADDR + SORTER_NUM_OUTLETS + i);
        }
        Serial.println("[SORTER] Servo positions loaded from EEPROM");
    }
    
    // 打印加载的舵机位置
    Serial.println("[SORTER] Loaded servo positions:");
    for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
        Serial.print("Outlet ");
        Serial.print(i);
        Serial.print(": Closed=");
        Serial.print(outletServoClosedPositions[i]);
        Serial.print(", Open=");
        Serial.println(outletServoOpenPositions[i]);
    }
    
    // 初始化所有出口并设置直径范围和舵机位置
    for (uint8_t i = 0; i < SORTER_NUM_OUTLETS; i++) {
        int minD = outletDiameterRanges[i][0];
        int maxD = outletDiameterRanges[i][1];
        int closedPos = outletServoClosedPositions[i];
        int openPos = outletServoOpenPositions[i];
        outlets[i].initialize(SERVO_PINS[i], minD, maxD, closedPos, openPos);
    }
    
    // 初始化出口位置
    uint8_t defaultDivergencePoints[SORTER_NUM_OUTLETS] = {1, 3, 5, 7, 9, 11, 13, 15};
    initializeDivergencePoints(defaultDivergencePoints);
    
    // 初始化托盘系统
    resetAllTraysData();
    
    // 设置编码器回调，将Sorter实例和静态回调函数连接到编码器
    encoder->setPhaseCallback(this, onEncoderPhaseChange);
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
        case 120:
            shouldCalculateDiameter = true;
            break;
        case 1:
            shouldRestartScan = true;
            break;
            
        // 出口相关操作
        case 110:
            resetOutlets = true;
            break;
        case 175:
            executeOutlets = true;
            break;
        default:
            // 无需处理的phase值
            break;
    }
    
    // 检查所有出口位置
}

// 实现静态回调函数
void Sorter::onEncoderPhaseChange(void* context, int phase) {
    // 将上下文转换回Sorter指针并调用成员函数
    Sorter* sorter = static_cast<Sorter*>(context);
    sorter->onPhaseChange(phase);
}

// 处理扫描仪相关任务
void Sorter::processScannerTasks() {
    if (shouldRestartScan) {
        shouldRestartScan = false;
        scanner->start();
    }
    
    if (shouldCalculateDiameter) {
        shouldCalculateDiameter = false;
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
        pushNewAsparagus(diameterMm, objectScanCount);
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
                if (adjustedPosition >= 0 && adjustedPosition < getCapacity()) {
                    int diameter = getTrayDiameter(adjustedPosition);
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
        
        // 出口状态已经通过出口舵机动作显示，无需额外LED指示
    }
    
    if (resetOutlets) {
        resetOutlets = false;
        for(int i = 0; i < SORTER_NUM_OUTLETS; i++){
            outlets[i].setReadyToOpen(false);
            outlets[i].execute();
        }
        // 复位出口状态已完成，无需额外LED指示
    }
}



// 出口控制公共方法实现
void Sorter::setOutletState(uint8_t outletIndex, bool open) {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex].setReadyToOpen(open);
        outlets[outletIndex].execute();
        // 公共方法中无需额外LED指示，出口状态已通过舵机动作显示
    }
}

// 获取特定出口对象的指针（用于诊断模式）
Outlet* Sorter::getOutlet(uint8_t index) {
    if (index < SORTER_NUM_OUTLETS) {
        return &outlets[index];
    }
    return nullptr;
}



// 实现预设出口功能
void Sorter::prepareOutlets() {
    // 出口0处理
    if (getTrayScanCount(0) > 1){
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
        if (outletPosition >= 0 && outletPosition < getCapacity()) {
            int diameter = getTrayDiameter(outletPosition);
            
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
    return getTrayDiameter(0);
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

// 获取出口最小直径
int Sorter::getOutletMinDiameter(uint8_t outletIndex) const {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        return outlets[outletIndex].getMatchDiameterMin();
    }
    return 0;
}

// 获取出口最大直径
int Sorter::getOutletMaxDiameter(uint8_t outletIndex) const {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        return outlets[outletIndex].getMatchDiameterMax();
    }
    return 0;
}

// 设置出口最小直径
void Sorter::setOutletMinDiameter(uint8_t outletIndex, int minDiameter) {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex].setMatchDiameterMin(minDiameter);
    }
}

// 设置出口最大直径
void Sorter::setOutletMaxDiameter(uint8_t outletIndex, int maxDiameter) {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex].setMatchDiameterMax(maxDiameter);
    }
}

// 获取出口关闭位置
int Sorter::getOutletClosedPosition(uint8_t outletIndex) const {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        return outlets[outletIndex].getClosedPosition();
    }
    return SERVO_CLOSED_POSITION;
}

// 获取出口打开位置
int Sorter::getOutletOpenPosition(uint8_t outletIndex) const {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        return outlets[outletIndex].getOpenPosition();
    }
    return SERVO_OPEN_POSITION;
}

// 设置出口关闭位置
void Sorter::setOutletClosedPosition(uint8_t outletIndex, int closedPosition) {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex].setClosedPosition(closedPosition);
    }
}

// 设置出口打开位置
void Sorter::setOutletOpenPosition(uint8_t outletIndex, int openPosition) {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex].setOpenPosition(openPosition);
    }
}

// 重置所有托盘数据
void Sorter::resetAllTraysData() {
    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        asparagusDiameters[i] = EMPTY_TRAY;
        asparagusCounts[i] = 0;
    }
}

// 获取托盘容量
uint8_t Sorter::getCapacity() {
    return QUEUE_CAPACITY;
}

// 获取指定托盘的直径
int Sorter::getTrayDiameter(int index) const {
    if (index >= 0 && index < QUEUE_CAPACITY) {
        return asparagusDiameters[index];
    }
    return EMPTY_TRAY;
}

// 获取指定托盘的扫描计数
int Sorter::getTrayScanCount(int index) const {
    if (index >= 0 && index < QUEUE_CAPACITY) {
        return asparagusCounts[index];
    }
    return 0;
}

// 向右移动所有托盘数据
void Sorter::shiftToRight() {
    for (int i = QUEUE_CAPACITY - 1; i > 0; i--) {
        asparagusDiameters[i] = asparagusDiameters[i - 1];
        asparagusCounts[i] = asparagusCounts[i - 1];
    }
    asparagusDiameters[0] = EMPTY_TRAY;
    asparagusCounts[0] = 0;
}

// 添加新的芦笋数据到托盘系统
void Sorter::pushNewAsparagus(int diameter, int scanCount) {
    shiftToRight();
    asparagusDiameters[0] = diameter;
    asparagusCounts[0] = scanCount;
}

