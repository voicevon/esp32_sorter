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
    
    // 初始化分流点索引为默认值
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
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

    // Call helper to restore/initialize outlet configuration
    restoreOutletConfig();
    
    // 初始化出口位置
    uint8_t defaultDivergencePoints[NUM_OUTLETS] = {1, 3, 5, 7, 9, 11, 13, 15};
    initializeDivergencePoints(defaultDivergencePoints);
    
    // 初始化托盘系统
    trayManager->resetAllTraysData();
    
    // 设置编码器回调，将Sorter实例和静态回调函数连接到编码器
    encoder->setPhaseCallback(this, onEncoderPhaseChange);
}

void Sorter::restoreOutletConfig() {
    // 定义EEPROM中存储直径范围的起始地址
    const int EEPROM_DIAMETER_RANGES_ADDR = 0;
    
    // 出口直径范围数组
    uint8_t outletDiameterRanges[NUM_OUTLETS][2];
    
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
        for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
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
    
    // 初始化所有出口并设置直径范围
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        // 先删除旧对象
        if (outlets[i]) {
            delete outlets[i];
            outlets[i] = nullptr;
        }

        int minD = outletDiameterRanges[i][0];
        int maxD = outletDiameterRanges[i][1];

        // 后续可以通过 HC595 输出，引脚参数暂时传入出口索引 i 交由底层处理
        // 注意：目前 Outlet 的构造函数可能还在期望真实的 GPIO
        // FIX: Pass -1 for pins to disable direct GPIO initialization as they conflict
        // with UART and will be driven by HC595 instead.
        Outlet* sol = new Outlet(-1, -1);
        sol->initialize();
        sol->setMatchDiameter(minD, maxD);
        outlets[i] = sol;
    }
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
    // 轮询更新所有出口状态 (处理电磁铁脉冲等时序)
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        if (outlets[i]) outlets[i]->update();
    }

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
                outlets[i]->setReadyToOpen(false);
                outlets[i]->execute();
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
                bool hasOutletOpened = false;
                for(int i = 0; i < NUM_OUTLETS; i++){
                    // 之前的逻辑保持不变
                     if (false) { // 预设出口0条件
                        outlets[i]->setReadyToOpen(true);
                        hasOutletOpened = true;
                    } else if (i > 0) {
                         int adjustedPosition = outletDivergencePoints[i] - 1;
                        if (adjustedPosition >= 0 && adjustedPosition < trayManager->getCapacity()) {
                            int diameter = trayManager->getTrayDiameter(adjustedPosition);
                            int minDiameter = outlets[i]->getMatchDiameterMin();
                            int maxDiameter = outlets[i]->getMatchDiameterMax();
                            
                            if ((i == 1 && diameter > minDiameter) || 
                                (i > 1 && diameter > minDiameter && diameter <= maxDiameter)) {
                                outlets[i]->setReadyToOpen(true);
                                hasOutletOpened = true;
                            } else {
                                outlets[i]->setReadyToOpen(false);
                            }
                        } else {
                            outlets[i]->setReadyToOpen(false);
                        }
                    } else {
                        outlets[i]->setReadyToOpen(false);
                    }
                    outlets[i]->execute();
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
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex]->setReadyToOpen(open);
        outlets[outletIndex]->execute();
        // 公共方法中无需额外LED指示，出口状态已通过舵机动作显示
    }
}

// 获取特定出口对象的指针（用于诊断模式）
Outlet* Sorter::getOutlet(uint8_t index) {
    if (index < SORTER_NUM_OUTLETS) {
        return outlets[index];
    }
    return nullptr;
}



// 实现预设出口功能
void Sorter::prepareOutlets() {
    // 出口0处理
    if (trayManager->getTrayScanCount(0) > 1){
        outlets[0]->setReadyToOpen(true);
    } else {
        outlets[0]->setReadyToOpen(false);
    }

    // 为每个出口预设状态
    for (uint8_t i = 1; i < SORTER_NUM_OUTLETS; i++) {
        // 修改：将出口位置索引减1，因为实际托盘计数从1开始
        int outletPosition = outletDivergencePoints[i] - 1;
        int minDiameter = outlets[i]->getMatchDiameterMin();
        int maxDiameter = outlets[i]->getMatchDiameterMax();
        
        // 确保调整后的索引在有效范围内
        if (outletPosition >= 0 && outletPosition < trayManager->getCapacity()) {
            int diameter = trayManager->getTrayDiameter(outletPosition);
            
            // 只在直径有效并且满足出口条件时设置出口状态
            if (diameter > 0 && diameter <= 50) {
                // 检查直径范围
                if ((i == 1 && diameter > minDiameter) || (i > 1 && diameter > minDiameter && diameter <= maxDiameter)) {
                    outlets[i]->setReadyToOpen(true);
                } else {
                    outlets[i]->setReadyToOpen(false);
                }
            } else {
                outlets[i]->setReadyToOpen(false);
            }
        } else {
            outlets[i]->setReadyToOpen(false);
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
    if (outletIndex < SORTER_NUM_OUTLETS) {
        return outlets[outletIndex]->getMatchDiameterMin();
    }
    return 0;
}

// 获取出口最大直径
int Sorter::getOutletMaxDiameter(uint8_t outletIndex) const {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        return outlets[outletIndex]->getMatchDiameterMax();
    }
    return 0;
}

// 设置出口最小直径
void Sorter::setOutletMinDiameter(uint8_t outletIndex, int minDiameter) {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex]->setMatchDiameterMin(minDiameter);
    }
}

// 设置出口最大直径
void Sorter::setOutletMaxDiameter(uint8_t outletIndex, int maxDiameter) {
    if (outletIndex < SORTER_NUM_OUTLETS) {
        outlets[outletIndex]->setMatchDiameterMax(maxDiameter);
    }
}


// 重置所有托盘数据
