#include "carriage_system.h"

// Carriage类实现
Carriage::Carriage(uint8_t idx) {
    index = idx;
    diameter = 0.0;
    isValid = false;
    isAssigned = false;
}

void Carriage::setDiameter(float diam) {
    diameter = diam;
    isValid = true;
    isAssigned = false;
}

float Carriage::getDiameter() const {
    return diameter;
}

void Carriage::setValid(bool valid) {
    isValid = valid;
}

bool Carriage::getValid() const {
    return isValid;
}

void Carriage::setAssigned(bool assigned) {
    isAssigned = assigned;
}

bool Carriage::getAssigned() const {
    return isAssigned;
}

uint8_t Carriage::getIndex() const {
    return index;
}

void Carriage::reset() {
    diameter = 0.0;
    isValid = false;
    isAssigned = false;
}

// CarriageManager类实现
CarriageManager::CarriageManager() {
    // 初始化所有托架
    for (uint8_t i = 0; i < TOTAL_CARRIAGES; i++) {
        carriages[i] = Carriage(i);
    }
    
    // 初始化分支点位置（默认值）
    for (uint8_t i = 0; i < NUM_DIVERTERS; i++) {
        divergencePointIndices[i] = 5 + i * 5; // 默认每5个位置一个分支点
    }
    
    shouldMoveCarriages = false;
}

void CarriageManager::setDivergencePoints(const uint8_t positions[NUM_DIVERTERS]) {
    for (uint8_t i = 0; i < NUM_DIVERTERS; i++) {
        if (positions[i] < TOTAL_CARRIAGES) {
            divergencePointIndices[i] = positions[i];
        }
    }
}

void CarriageManager::addNewDiameterData(float diameter) {
    // 首先移动所有现有托架数据
    moveCarriages();
    
    // 在索引0位置添加新数据
    carriages[0].setDiameter(diameter);
    
    Serial.print("添加新数据到索引0: 直径 = ");
    Serial.println(diameter);
}

void CarriageManager::moveCarriages() {
    // 从最后一个托架开始，向前移动数据
    carriages[TOTAL_CARRIAGES - 1].reset(); // 最后一个托架数据丢弃
    
    for (int8_t i = TOTAL_CARRIAGES - 2; i >= 0; i--) {
        if (carriages[i].getValid()) {
            // 将当前托架数据复制到下一个位置
            carriages[i + 1].setDiameter(carriages[i].getDiameter());
            carriages[i + 1].setAssigned(carriages[i].getAssigned());
        } else {
            carriages[i + 1].reset();
        }
        // 重置当前位置
        carriages[i].reset();
    }
    
    Serial.println("所有托架数据已移动");
}

uint8_t CarriageManager::checkAndExecuteAssignment(uint8_t currentIndex) {
    // 检查当前位置是否是分支点
    for (uint8_t i = 0; i < NUM_DIVERTERS; i++) {
        if (currentIndex == divergencePointIndices[i]) {
            Carriage& carriage = carriages[currentIndex];
            
            // 检查托架是否有效且未分配
            if (carriage.getValid() && !carriage.getAssigned()) {
                // 确定应该分配到哪个出口
                uint8_t outlet = determineOutlet(carriage.getDiameter());
                
                // 如果出口匹配当前分支器，执行分配
                if (outlet == i + 1) { // 分支器索引从0开始，出口编号从1开始
                    carriage.setAssigned(true);
                    Serial.print("在分支点 ");
                    Serial.print(currentIndex);
                    Serial.print(" 分配托架: 直径 = ");
                    Serial.print(carriage.getDiameter());
                    Serial.print(", 出口 = ");
                    Serial.println(outlet);
                    return outlet;
                }
            }
        }
    }
    
    return 0; // 不需要动作
}

Carriage& CarriageManager::getCarriage(uint8_t index) {
    if (index < TOTAL_CARRIAGES) {
        return carriages[index];
    }
    // 如果索引无效，返回第一个托架（应该避免这种情况）
    return carriages[0];
}

void CarriageManager::resetAllCarriages() {
    for (uint8_t i = 0; i < TOTAL_CARRIAGES; i++) {
        carriages[i].reset();
    }
    Serial.println("所有托架已重置");
}

uint8_t CarriageManager::determineOutlet(float diameter) {
    // 异常处理：无效数据默认到第5个出口
    if (diameter < 0 || diameter > 25.0) {
        return 5; // 默认出口
    }
    
    // 级联式分配逻辑
    if (diameter > DIAMETER_THRESHOLD_1) {
        return 1; // 出口1: >15mm
    } else if (diameter > DIAMETER_THRESHOLD_2) {
        return 2; // 出口2: 12-15mm
    } else if (diameter > DIAMETER_THRESHOLD_3) {
        return 3; // 出口3: 9-12mm
    } else if (diameter > DIAMETER_THRESHOLD_4) {
        return 4; // 出口4: 6-9mm
    } else {
        return 5; // 出口5: ≤6mm
    }
}