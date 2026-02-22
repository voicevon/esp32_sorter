#include "sorter.h"
#include "HardwareSerial.h"
#include "../config.h"
#include "tray_system.h"
#include <Arduino.h>
#include <cstddef>
#include <EEPROM.h>

Sorter::Sorter() :
                   currentState(STATE_IDLE),
                   lastSpeedCheckTime(0), lastEncoderPosition(0), lastSpeed(0.0f), lastObjectCount(0) {
    // 构造函数初始化 - 使用单例模式获取实例
    encoder = Encoder::getInstance();
    simpleHmi = SimpleHMI::getInstance();
    trayManager = TraySystem::getInstance();
    
    // 构造函数仅进行基础变量重置，所有硬件和业务参数初始化统一由 initialize() 处理
}

void Sorter::initialize() {
    // 1. 初始化出口间的默认几何分流点
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        outletDivergencePoints[i] = 1 + i * 2; 
        outlets[i].initialize();
    }
    // 初始化出口位置
    uint8_t defaultDivergencePoints[NUM_OUTLETS] = {1, 3, 5, 7, 9, 11, 13, 15};
    initializeDivergencePoints(defaultDivergencePoints);
    
    // 初始化托盘系统
    trayManager->resetAllTraysData();
    
    // 初始化 74HC595 引脚
    pinMode(PIN_HC595_DS, OUTPUT);
    pinMode(PIN_HC595_SHCP, OUTPUT);
    pinMode(PIN_HC595_STCP, OUTPUT);
    digitalWrite(PIN_HC595_STCP, LOW);
    
    // 初始化同步输出
    updateShiftRegisters();

    // 设置编码器回调，将Sorter实例和静态回调函数连接到编码器
    encoder->setPhaseCallback(this, onEncoderPhaseChange);
}

void Sorter::restoreOutletConfig() {
    // 默认直径范围（如果EEPROM为空时使用）
    int outletDiameterRanges[NUM_OUTLETS][2] = {
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
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        int minD = outletDiameterRanges[i][0];
        int maxD = outletDiameterRanges[i][1];

        outlets[i].initialize();
        outlets[i].setMatchDiameter(minD, maxD);
    }
}

// 初始化出口位置实现
void Sorter::initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]) {
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        if (positions[i] < 31) { // TOTAL_TRAYS = 31
            outletDivergencePoints[i] = positions[i];
        } else {
            // 如果提供的位置无效，使用默认值
            outletDivergencePoints[i] = 1 + i * 2;
        }
    }
}

void Sorter::onPhaseChange(int phase) {
    // 编码器中断始终工作
    // 状态转换逻辑
    scanner->sample(phase);  // 保持采样调用（内部已优化为轻量级）
    
    switch (phase) {
        case PHASE_SCAN_START: // 1
            currentState = STATE_SCANNING;
            break;
            
        case PHASE_RESET_OUTLETS_LEGACY: // 110
            currentState = STATE_RESETTING_OUTLETS;
            break;
            
        case PHASE_CALC_DIAMETER_LEGACY: // 120
            currentState = STATE_CALCULATING_DIAMETER;
            break;

        case PHASE_EXECUTE_OUTLETS_LEGACY: // 175
            currentState = STATE_EXECUTING_OUTLETS;
            break;
            
        default:
            // 其他相位保持当前状态或在run()中自动流转
            break;
    }
}

// 实现静态回调函数
void Sorter::onEncoderPhaseChange(void* context, int phase) {
    // 将上下文转换回Sorter指针并调用成员函数
    Sorter* sorter = static_cast<Sorter*>(context);
    sorter->onPhaseChange(phase);
}

// 主循环处理函数 (FSM Executor)
void Sorter::run() {
    // 轮询物理出口的逻辑状态更新 (处理 H 桥脉冲等时序)
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        outlets[i].update();
    }
    
    // 关键原子操作：将 24 位逻辑映射推送到级联 HC595
    updateShiftRegisters();

    switch (currentState) {
        case STATE_SCANNING:
            // 在扫描状态下，确保扫描仪开启
            // 注意：start() 会重置数据，所以只需调用一次。
            // 这里我们依赖状态转换的瞬时性，或者可以在onPhaseChange中直接调用start()
            // 为了安全，我们在进入Scanning状态时的第一个循环调用start
            // 但更好的方式是：在onPhaseChange中设置状态时，让run()去处理"Entry Action"
            // 由于onPhaseChange是中断，我们不做重操作。
            // 我们使用一个静态变量或检查scanner是否已经在扫描
            // 简单起见，我们在PHASE_SCAN_START时设置状态，run()检测到状态变化执行动作
            // 这里简化为：每次run检查状态并执行相应逻辑
            
            // 实际上，扫描仪的start/stop应该由状态机管理
            // PHASE 1 -> STATE_SCANNING
            if (!scanner->isScanningActive()) { // 假设添加了isScanningActive方法，或者直接调用start()如果不影响
                 scanner->start();
            }
            break;

        case STATE_RESETTING_OUTLETS:
            // 对应 resetOutlets
             for(int i = 0; i < NUM_OUTLETS; i++){
                outlets[i].setReadyToOpen(false);
                outlets[i].execute();
            }
            // 执行完后，可以转回IDLE或者保持直到下一个相位
            currentState = STATE_IDLE; 
            break;

        case STATE_CALCULATING_DIAMETER:
            // 对应 shouldCalculateDiameter
            {
                // 从扫描仪获取并计算直径
                int diameterMm = scanner->getDiameterAndStop();
                int objectScanCount = scanner->getTotalObjectCount();
                
                if (diameterMm > 0) {
                    Serial.print("原始值检测到: ");
                    Serial.print(diameterMm);
                    Serial.print("mm, 物体计数: ");
                    Serial.println(objectScanCount);
                }
                
                // 推送到托盘系统
                trayManager->pushNewAsparagus(diameterMm, objectScanCount);
                // 预计算出口状态
                prepareOutlets();
                
                // 计算完成后，状态流转到IDLE等待下一个相位
                currentState = STATE_IDLE;
            }
            break;

        case STATE_EXECUTING_OUTLETS:
             // 对应 executeOutlets
            {
                for (int i = 0; i < NUM_OUTLETS; i++) {
                int adjustedPosition = outletDivergencePoints[i];
                if (adjustedPosition >= 0 && adjustedPosition < trayManager->getCapacity()) {
                    int diameter = trayManager->getTrayDiameter(adjustedPosition);
                    int minD = outlets[i].getMatchDiameterMin();
                    int maxD = outlets[i].getMatchDiameterMax();
                    
                    if (diameter > 0 && ((i == 1 && diameter > minD) || (i > 1 && diameter > minD && diameter <= maxD))) {
                        outlets[i].setReadyToOpen(true);
                    } else {
                        outlets[i].setReadyToOpen(false);
                    }
                }
                outlets[i].execute();
            }
            currentState = STATE_IDLE;
            }
            break;

        case STATE_IDLE:
        default:
            // 空闲状态，什么都不做
            break;
    }
}



// 出口控制公共方法实现
void Sorter::setOutletState(uint8_t outletIndex, bool open) {
    outlets[outletIndex].setReadyToOpen(open);
    outlets[outletIndex].execute();
}

// 获取特定出口对象的指针（用于诊断模式同步，返回地址）
Outlet* Sorter::getOutlet(uint8_t index) {
    return &outlets[index];
}



// 实现预设出口功能
void Sorter::prepareOutlets() {
    // 出口0处理
    if (trayManager->getTrayScanCount(0) > 1){
        outlets[0].setReadyToOpen(true);
    } else {
        outlets[0].setReadyToOpen(false);
    }

    // 为每个出口计算对应的托盘位置并执行
    for (int i = 0; i < NUM_OUTLETS; i++) {
        // 出口对应的托盘偏移
        int outletPosition = outletDivergencePoints[i];
        
        int minDiameter = outlets[i].getMatchDiameterMin();
        int maxDiameter = outlets[i].getMatchDiameterMax();
        
        // 确保调整后的索引在有效范围内
        if (outletPosition >= 0 && outletPosition < trayManager->getCapacity()) {
            int diameter = trayManager->getTrayDiameter(outletPosition);
            
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
    return trayManager->getTrayDiameter(0);
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
    return outlets[outletIndex].getMatchDiameterMin();
}

// 获取出口最大直径
int Sorter::getOutletMaxDiameter(uint8_t outletIndex) const {
    return outlets[outletIndex].getMatchDiameterMax();
}

// 设置出口最小直径
void Sorter::setOutletMinDiameter(uint8_t outletIndex, int minDiameter) {
    if (outletIndex < NUM_OUTLETS) {
        outlets[outletIndex].setMatchDiameterMin(minDiameter);
    }
}

// 设置出口最大直径
void Sorter::setOutletMaxDiameter(uint8_t outletIndex, int maxDiameter) {
    if (outletIndex < NUM_OUTLETS) {
        outlets[outletIndex].setMatchDiameterMax(maxDiameter);
    }
}


void Sorter::updateShiftRegisters() {
    uint8_t ledByte = 0;      // Byte 0 (Index 0): 8个 LED
    uint8_t chip1Byte = 0;    // Byte 1 (Index 1): 出口 0-3 的 H 桥对
    uint8_t chip2Byte = 0;    // Byte 2 (Index 2): 出口 4-7 的 H 桥对
    
    // 构建 LED 指示灯位图 (反映当前物理位置)
    for (int i = 0; i < NUM_OUTLETS; i++) {
        if (outlets[i].isPositionOpen()) {
            ledByte |= (1 << i);
        }
    }

    // 构建电磁铁 H 桥位图 (每 2 位控制一个电磁铁)
    // Chip 1 控制出口 0-3
    for (int i = 0; i < 4; i++) {
        if (outlets[i].isOpenPulseActive()) {
            chip1Byte |= (1 << (i * 2));     // Bit 0, 2, 4, 6 为 Open 信号
        }
        if (outlets[i].isClosePulseActive()) {
            chip1Byte |= (1 << (i * 2 + 1)); // Bit 1, 3, 5, 7 为 Close 信号
        }
    }
    
    // Chip 2 控制出口 4-7
    for (int i = 0; i < 4; i++) {
        if (outlets[i + 4].isOpenPulseActive()) {
            chip2Byte |= (1 << (i * 2));     // 对位到 Chip 2 的 Bit 0, 2, 4, 6
        }
        if (outlets[i + 4].isClosePulseActive()) {
            chip2Byte |= (1 << (i * 2 + 1)); // 对位到 Chip 2 的 Bit 1, 3, 5, 7
        }
    }
    
    // 组合成 24 位数据以进行变化检测
    uint32_t currentData = ((uint32_t)chip2Byte << 16) | ((uint32_t)chip1Byte << 8) | ledByte;
    
    // 仅在状态变化时刷新物理引脚
    if (currentData != lastShiftData) {
        // 如果有任何脉冲处于活动状态，打印详细的出口信息
        if (chip1Byte != 0 || chip2Byte != 0) {
            String activeOutlets = "";
            for(int i=0; i<NUM_OUTLETS; i++) {
                if(outlets[i].isOpenPulseActive()) activeOutlets += "O" + String(i) + "(Open) ";
                if(outlets[i].isClosePulseActive()) activeOutlets += "O" + String(i) + "(Close) ";
            }
            Serial.printf("[595] Data Change! -> 24-bit: 0x%06X | Active: %s\n", currentData, activeOutlets.c_str());
        } else {
            Serial.printf("[595] Data Change! -> 24-bit: 0x%06X (Idle Mode, LEDs Only)\n", currentData);
        }
                      
        digitalWrite(PIN_HC595_STCP, LOW);
        
        // 发送顺序：最先发出的字节会被推到级联链路的最末端 (Chip 2)
        // 1. 发送 Chip 2 数据 (出口 4-7)
        shiftOut(PIN_HC595_DS, PIN_HC595_SHCP, MSBFIRST, chip2Byte);
        // 2. 发送 Chip 1 数据 (出口 0-3)
        shiftOut(PIN_HC595_DS, PIN_HC595_SHCP, MSBFIRST, chip1Byte);
        // 3. 发送 Chip 0 数据 (LED 指示灯)
        shiftOut(PIN_HC595_DS, PIN_HC595_SHCP, MSBFIRST, ledByte);
        
        digitalWrite(PIN_HC595_STCP, HIGH);
        lastShiftData = currentData;
    }
}
