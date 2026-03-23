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
    lastObjectCount(0) 
{
    // 实例化互斥锁
    mutex = xSemaphoreCreateMutex();

    // 实例获取
    encoder = Encoder::getInstance();
    simpleHmi = SimpleHMI::getInstance();
    trayManager = TraySystem::getInstance();
    scanner = DiameterScanner::getInstance(); // 初始化scanner指针，防止空指针异常
    
    // 构造函数仅进行基础变量重置，所有硬件和业务参数初始化统一由 initialize() 处理
}

void Sorter::initialize() {
    // 1. 初始化 74HC595 引脚（必须在初始化物理出口前完成，才能推入数据）
    pinMode(PIN_HC595_DS, OUTPUT);
    pinMode(PIN_HC595_SHCP, OUTPUT);
    pinMode(PIN_HC595_STCP, OUTPUT);
    digitalWrite(PIN_HC595_STCP, LOW);

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
}

void Sorter::restoreOutletConfig() {
    EEPROM.begin(512);
    const int EEPROM_DIAMETER_RANGES_ADDR = 0;
    
    // 检查 EEPROM 是否已初始化
    if (EEPROM.read(EEPROM_DIAMETER_RANGES_ADDR) == 0xAA) {
        Serial.println("[Sorter] Restoring configuration from EEPROM...");
        // 读取直径配置
        for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
            int minD = EEPROM.read(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2);
            int maxD = EEPROM.read(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2 + 1);
            outlets[i].setMatchDiameter(minD, maxD);
        }
    } else {
        Serial.println("[Sorter] EEPROM empty, using default ranges.");
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
        flagScanStart = false; // 清理标志位，确保只处理一次
        Serial.println("[SORTER] Event -> START SCAN (50)");
    } 

    // B. 数据锁存阶段 (170) -> 这里完成物体的判定
    if (flagDataLatch) {
        int diameterMm = scanner->getDiameterAndStop();
        int objectCount = scanner->getTotalObjectCount();
        
        if (diameterMm > 0) {
            Serial.printf("[SORTER] Event -> LATCH DATA (170) | Diam: %d mm\n", diameterMm);
        }
        
        // 推送到托盘系统的起始端
        trayManager->pushNewAsparagus(diameterMm, objectCount);
        prepareOutlets(); // 预计算出口状态
        
        flagDataLatch = false;
    }

    // C. 执行分拣动作 (30)
    if (flagOutletExecute) {
        for (int i = 0; i < NUM_OUTLETS; i++) {
            outlets[i].execute(); // 根据 170 相位算好的状态触发电磁铁动作
        }
        
        flagOutletExecute = false;
        Serial.println("[SORTER] Event -> EXECUTE OUTLETS (30)");
    }

    // D. 重置/关闭驱动信号 (150)
    if (flagOutletReset) {
         for(int i = 0; i < NUM_OUTLETS; i++){
            if (outlets[i].shouldStayOpenNext()) {
                // Serial.printf("[SORTER] Predictive Action -> Stay OPEN for outlet %d\n", i);
                continue; 
            }
            outlets[i].setReadyToOpen(false);
            outlets[i].execute();
        }
        flagOutletReset = false;
        Serial.println("[SORTER] Event -> RESET OUTLETS (150)");
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
    
    // 定义统一的匹配判定逻辑 (Lambda)，确保当前和前瞻使用同一套规则
    auto isMatch = [&](int p, int outletIdx) -> bool {
        if (p < 0 || p >= capacity) return false;
        
        if (outletIdx == 0) {
            // 出口 0：处理多物体/碎料/重叠
            return trayManager->getTrayScanCount(p) > 1;
        } else {
            // 出口 1-7：直径分级
            int diameter = trayManager->getTrayDiameter(p);
            int minD = outlets[outletIdx].getMatchDiameterMin();
            int maxD = outlets[outletIdx].getMatchDiameterMax();
            
            if (diameter <= 0 || diameter > 50) return false;
            
            // 出口 1 为 > minD；出口 2-7 为 (minD, maxD]
            if (outletIdx == 1) return diameter > minD;
            return (diameter > minD && diameter <= maxD);
        }
    };

    for (int i = 0; i < NUM_OUTLETS; i++) {
        int pos = outletDivergencePoints[i];
        
        // 1. 确定当前时刻该出口是否需要打开
        bool currentMatch = isMatch(pos, i);

        // 2. [预判前瞻] 判定紧随其后的托盘 (pos-1) 是否也需要进该出口
        // 如果连续两个托盘合意，则标记 stayOpenNext，指示 150 相位跳过复位
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
    float val = 0.0f;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        val = lastSpeed;
        xSemaphoreGive(mutex);
    }
    return val;
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
    
    // 组合成 24 位数据以进行变化检测
    uint32_t currentData = ((uint32_t)chip2Byte << 16) | ((uint32_t)chip1Byte << 8) | ledByte;
    
    // 仅在状态变化时刷新物理引脚
    if (currentData != lastShiftData) {
        // 如果有任何脉冲处于活动状态，打印详细的出口信息
        if (chip1Byte != 0 || chip2Byte != 0) {
            Serial.printf("[595] Data Change! -> 24-bit: 0x%06X | Active: ", currentData);
            for(int i=0; i<NUM_OUTLETS; i++) {
                if(outlets[i].isOpenPulseActive()) Serial.printf("O%d(Open) ", i);
                if(outlets[i].isClosePulseActive()) Serial.printf("O%d(Close) ", i);
            }
            Serial.println();
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
void Sorter::saveConfig() {
    Serial.println("[Sorter] Saving configuration to EEPROM...");
    EEPROM.begin(512);
    const int EEPROM_DIAMETER_RANGES_ADDR = 0;
    EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR, 0xAA);
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2, outlets[i].getMatchDiameterMin());
        EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2 + 1, outlets[i].getMatchDiameterMax());
    }
    
    EEPROM.commit();
    Serial.println("[Sorter] Configuration saved successfully.");
}
