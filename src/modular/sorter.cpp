#include "sorter.h"
#include "HardwareSerial.h"
#include "user_interface/oled.h"
#include "../config.h"
#include "tray_system.h"
#include <Arduino.h>
#include <cstddef>
#include <EEPROM.h>

Sorter::Sorter() :
    flagScanStart(false), 
    flagDataLatch(false), 
    flagOutletExecute(false), 
    flagOutletReset(false),
    lastSpeedCheckTime(0),
    lastEncoderPosition(0), 
    lastSpeed(0.0f), 
    lastObjectCount(0),
    shiftDriver(PIN_HC595_DS, PIN_HC595_SHCP, PIN_HC595_STCP)
{
    // 实例化互斥锁
    mutex = xSemaphoreCreateMutex();

    // 实例获取
    encoder = Encoder::getInstance();
    simpleHmi = RotaryInputSource::getInstance();
    trayManager = TraySystem::getInstance();
    scanner = DiameterScanner::getInstance(); // 初始化scanner指针，防止空指针异常
    
    // 构造函数仅进行基础变量重置，所有硬件和业务参数初始化统一由 initialize() 处理
}

void Sorter::initialize() {
    // 1. 初始化硬件驱动
    shiftDriver.initialize();

    // 2. 初始化出口位置
    uint8_t defaultDivergencePoints[NUM_OUTLETS] = {0, 2, 4, 6, 8, 10, 12, 14};
    initializeDivergencePoints(defaultDivergencePoints);
    
    // 3. 初始化出口逻辑状态（仅内存状态确立，移除阻塞式等待动作）
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        outlets[i].initialize();
    }
    
    // 执行一次物理引脚同步
    updateShiftRegisters();
    Serial.println("Sorter components initialized (Software states set).");
    
    // 初始化托盘系统
    trayManager->resetAllTraysData();

    // 设置编码器回调，将Sorter实例和静态回调函数连接到编码器
    encoder->setPhaseCallback(this, onEncoderPhaseChange);

    // 从 EEPROM 恢复出口直径配置
    restoreOutletConfig();
}

void Sorter::restoreOutletConfig() {
    // 检查 EEPROM 是否已初始化
    if (EEPROM.read(EEPROM_ADDR_DIAMETER) == 0xAA) {
        Serial.println("[Sorter] Restoring configuration from EEPROM...");
        // 读取出口配置 (每个出口占 3 字节: Min, Max, Length)
        for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
            int minD = EEPROM.read(EEPROM_ADDR_DIAMETER_DATA + i * 3);
            int maxD = EEPROM.read(EEPROM_ADDR_DIAMETER_DATA + i * 3 + 1);
            int lenL = EEPROM.read(EEPROM_ADDR_DIAMETER_DATA + i * 3 + 2);
            outlets[i].setMatchDiameter(minD, maxD);
            // 修正：掩码最大值为 7 (LEN_ALL)，若读取到非法值则默认为 LEN_ALL
            outlets[i].setTargetLength(lenL > 7 ? LEN_ALL : lenL);
        }
        // 读取出口 0 模式
        outlet0Mode = EEPROM.read(EEPROM_ADDR_OUTLET0_MODE);
        if (outlet0Mode > 1) outlet0Mode = 0;
    } else {
        Serial.println("[Sorter] EEPROM empty, using default ranges.");
        // 默认直径范围
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
        for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
            outlets[i].setMatchDiameter(outletDiameterRanges[i][0], outletDiameterRanges[i][1]);
        }
    }

    // 初始化所有出口逻辑
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        outlets[i].initialize();
    }
}

// 初始化出口位置实现
void Sorter::initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]) {
    uint8_t capacity = TraySystem::getCapacity();
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        if (positions[i] < capacity) { 
            outletDivergencePoints[i] = positions[i];
        } else {
            // 如果提供的位置无效，使用默认值
            outletDivergencePoints[i] = 1 + i * 2;
        }
    }
}

void Sorter::onPhaseChange(int phase) {
    // 1. 实时采样（必须在中断中完成）
    scanner->sample(phase); 
    
    // 2. 标志位置位（原子化记录事件，等待 run() 处理）
    switch (phase) {
        case PHASE_SCAN_START:    flagScanStart = true;     break;
        case PHASE_DATA_LATCH:    
            scanner->stop(); // 关键：立即在中断中停止，彻底解决因任务调度延迟导致的计数溢出
            flagDataLatch = true;     
            break;
        case PHASE_OUTLET_EXECUTE: flagOutletExecute = true; break;
        case PHASE_OUTLET_RESET:   flagOutletReset = true;   break;
        default: break;
    }
}

// 实现静态回调函数
void Sorter::onEncoderPhaseChange(void* context, int phase) {
    Sorter* sorter = static_cast<Sorter*>(context);
    sorter->onPhaseChange(phase);
}

// 主循环处理函数 (事件驱动消费)
void Sorter::run() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) != pdTRUE) return;

    // 1. 速度更新逻辑（每 100ms 计算一次）
    if (millis() - lastSpeedCheckTime >= 100) {
        lastSpeedCheckTime = millis();
        long currentPulse = encoder->getRawCount();
        long diff = currentPulse - lastEncoderPosition;
        lastEncoderPosition = currentPulse;
        lastSpeed = (float)diff / 20.0f; 
    }

    // 2. 异步事件消费 (处理由 onPhaseChange 置位的标志位)

    // A. 启动扫描阶段 (50)
    if (flagScanStart) {
        scanner->start();
        flagScanStart = false;
    } 

    // B. 数据锁存阶段 (170) -> 这里完成物体的判定
    if (flagDataLatch) {
        int diameterMm = scanner->getDiameterAndStop();
        int objectCount = scanner->getTotalObjectCount();
        int lengthLevel = scanner->getLengthLevel();
        
        // 推送到托盘系统的起始端
        trayManager->pushNewAsparagus(diameterMm, objectCount, lengthLevel);
        prepareOutlets(); // 预计算出口状态
        
        flagDataLatch = false;
    }

    // C. 执行分拣动作 (30)
    if (flagOutletExecute) {
        for (int i = 0; i < NUM_OUTLETS; i++) {
            outlets[i].execute();
        }
        flagOutletExecute = false;
    }

    // D. 重置/关闭驱动信号 (150)
    if (flagOutletReset) {
        for (int i = 0; i < NUM_OUTLETS; i++) {
            if (outlets[i].shouldStayOpenNext()) continue;
            outlets[i].setReadyToOpen(false);
            outlets[i].execute();
        }
        flagOutletReset = false;
    }

    // 3. 通用物理更新（每帧执行，处理脉冲宽度管理）
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        outlets[i].update();
    }
    updateShiftRegisters();

    xSemaphoreGive(mutex);
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
    uint8_t capacity = TraySystem::getCapacity();
    int asparagusLength = scanner->getLengthLevel();

    // 定义统一的匹配判定逻辑 (Lambda)
    auto isMatch = [&](int p, int outletIdx) -> bool {
        if (p < 0 || p >= capacity) return false;
        
        if (outletIdx == 0 && outlet0Mode == 0) {
            // 出口 0 的特殊识别模式：多物体/碎料检测
            return trayManager->getTrayScanCount(p) > 1;
        }

        // 通用直径匹配
        int diameter = trayManager->getTrayDiameter(p);
        if (diameter <= 0) return false; // 排除空位或无效数据

        int minD = outlets[outletIdx].getMatchDiameterMin();
        int maxD = outlets[outletIdx].getMatchDiameterMax();
        
        // 统一区间判定 (minD < diameter <= maxD)
        if (diameter <= minD || diameter > maxD) return false;

        // 【新增阶段】长度匹配 (位掩码逻辑)
        int detectedLength = trayManager->getTrayLengthLevel(p);
        uint8_t targetLenMask = outlets[outletIdx].getTargetLength();
        if (targetLenMask != LEN_ALL && targetLenMask != LEN_NONE) {
            // detectedLength 也是一个 LengthMask (LEN_S, LEN_M, LEN_L)
            // 直接通过位与运算判定是否在允许范围内
            if (!(targetLenMask & detectedLength)) return false;
        }

        return true;
    };

    for (int i = 0; i < NUM_OUTLETS; i++) {
        int pos = outletDivergencePoints[i];
        
        bool currentMatch = isMatch(pos, i);
        // [预判前瞻] 此处暂不考虑长度前瞻，仅以直径合意为准或需精确判定
        bool nextMatch = isMatch(pos - 1, i);

        outlets[i].setReadyToOpen(currentMatch);
        outlets[i].setStayOpenNext(currentMatch && nextMatch);
    }
}

// 获取最新直径
int Sorter::getLatestDiameter() {
    int val = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        val = trayManager->getTrayDiameter(0);
        xSemaphoreGive(mutex);
    }
    return val;
}

// 获取已经输送的托架数量
int Sorter::getTransportedTrayCount() {
    // 每一个托架固定对应 200 个脉冲（ pulsesPerTray ）
    return encoder->getRawCount() / 200;
    // 注：此计数来自编码器，编码器本身是原子读取，无需 Sorter 互斥锁保护
}

// 获取传送带速度（托架/秒）- 返回float类型
float Sorter::getConveyorSpeedPerSecond() {
    return lastSpeed.load(); // 原子读取，无需互斥锁保护
}

// 获取显示数据（用于 UserInterface）
// Sorter::getDisplayData方法已移除，改用专用方法
// 获取出口最小直径
int Sorter::getOutletMinDiameter(uint8_t outletIndex) {
    int val = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        if (outletIndex < NUM_OUTLETS) {
            val = outlets[outletIndex].getMatchDiameterMin();
        }
        xSemaphoreGive(mutex);
    }
    return val;
}

// 获取出口最大直径
int Sorter::getOutletMaxDiameter(uint8_t outletIndex) {
    int val = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        if (outletIndex < NUM_OUTLETS) {
            val = outlets[outletIndex].getMatchDiameterMax();
        }
        xSemaphoreGive(mutex);
    }
    return val;
}

// 设置出口最小直径
void Sorter::setOutletMinDiameter(uint8_t outletIndex, int minDiameter) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (outletIndex < NUM_OUTLETS) {
            outlets[outletIndex].setMatchDiameterMin(minDiameter);
        }
        xSemaphoreGive(mutex);
    }
}

// 设置出口最大直径
void Sorter::setOutletMaxDiameter(uint8_t outletIndex, int maxDiameter) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (outletIndex < NUM_OUTLETS) {
            outlets[outletIndex].setMatchDiameterMax(maxDiameter);
        }
        xSemaphoreGive(mutex);
    }
}


void Sorter::updateShiftRegisters() {
    uint8_t ledByte = 0;      // Byte 0 (Index 0): 8个 LED
    uint8_t chip1Byte = 0;    // Byte 1 (Index 1): 出口 0-3 的 H 桥对
    uint8_t chip2Byte = 0;    // Byte 2 (Index 2): 出口 4-7 的 H 桥对
    
    // 构建 LED 指示灯位图 (反映当前物理位置)
    for (int i = 0; i < NUM_OUTLETS; i++) {
        if (outlets[i].isPositionOpen()) {
            // 硬件映射修正：0-3 位正常映射，4-7 位反向映射 (7-6-5-4)
            if (i < 4) {
                ledByte |= (1 << i);
            } else {
                // i=4 -> bit 7, i=5 -> bit 6, i=6 -> bit 5, i=7 -> bit 4
                ledByte |= (1 << (7 - (i - 4)));
            }
        }
    }

    // 构建电磁铁 H 桥位图 (每 2 位控制一个电磁铁)
    // 根据硬件修正：Chip 1 控制出口 4-7
    for (int i = 0; i < 4; i++) {
        if (outlets[i + 4].isOpenPulseActive()) {
            chip1Byte |= (1 << (i * 2));     // Bit 0, 2, 4, 6 为 Open 信号
        }
        if (outlets[i + 4].isClosePulseActive()) {
            chip1Byte |= (1 << (i * 2 + 1)); // Bit 1, 3, 5, 7 为 Close 信号
        }
    }
    
    // 根据硬件修正：Chip 2 控制出口 0-3
    for (int i = 0; i < 4; i++) {
        if (outlets[i].isOpenPulseActive()) {
            chip2Byte |= (1 << (i * 2));     // 对位到 Chip 2 的 Bit 0, 2, 4, 6
        }
        if (outlets[i].isClosePulseActive()) {
            chip2Byte |= (1 << (i * 2 + 1)); // 对位到 Chip 2 的 Bit 1, 3, 5, 7
        }
    }
    
    // 物理刷新 (由 Driver 负责脏检查)
    shiftDriver.write(chip2Byte, chip1Byte, ledByte);
}
void Sorter::saveConfig() {
    Serial.println("[Sorter] Saving configuration to EEPROM...");
    EEPROM.write(EEPROM_ADDR_DIAMETER, 0xAA);
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        EEPROM.write(EEPROM_ADDR_DIAMETER_DATA + i * 3,     outlets[i].getMatchDiameterMin());
        EEPROM.write(EEPROM_ADDR_DIAMETER_DATA + i * 3 + 1, outlets[i].getMatchDiameterMax());
        EEPROM.write(EEPROM_ADDR_DIAMETER_DATA + i * 3 + 2, outlets[i].getTargetLength());
    }
    EEPROM.write(EEPROM_ADDR_OUTLET0_MODE, outlet0Mode);
    EEPROM.commit();
    Serial.println("[Sorter] Configuration saved successfully.");
}
