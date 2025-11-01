#include "carriage_system.h"

// 全局直径数据数组定义
float carriageDiameters[TOTAL_CARRIAGES];

// 全局出口位置数组定义
uint8_t divergencePointIndices[NUM_OUTLETS];

/**
 * 初始化出口位置
 * @param positions 出口位置数组
 */
void initializeDivergencePoints(const uint8_t positions[NUM_OUTLETS]) {
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        if (positions[i] < TOTAL_CARRIAGES) {
            divergencePointIndices[i] = positions[i];
        } else {
            // 如果提供的位置无效，使用默认值
            divergencePointIndices[i] = 5 + i * 5;
        }
    }
    
    // 初始重置所有直径数据
    resetAllCarriagesData();
}

/**
 * 从单点扫描仪添加新的直径数据（插入到索引0）
 * @param diameter 直径值
 */
void addNewDiameterData(float diameter) {
    // 首先移动所有现有数据
    moveCarriagesData();
    
    // 在索引0位置添加新数据
    carriageDiameters[0] = diameter;
    
    Serial.print("添加新数据到索引0: 直径 = ");
    Serial.println(diameter);
}

/**
 * 移动所有托架数据（索引值+1）
 */
void moveCarriagesData() {
    // 从最后一个位置开始，向前移动数据
    carriageDiameters[TOTAL_CARRIAGES - 1] = INVALID_DIAMETER; // 最后一个位置数据丢弃
    
    for (int8_t i = TOTAL_CARRIAGES - 2; i >= 0; i--) {
        if (carriageDiameters[i] != INVALID_DIAMETER) {
            // 将当前位置数据复制到下一个位置
            carriageDiameters[i + 1] = carriageDiameters[i];
        } else {
            carriageDiameters[i + 1] = INVALID_DIAMETER;
        }
        // 重置当前位置
        carriageDiameters[i] = INVALID_DIAMETER;
    }
    
    Serial.println("所有直径数据已移动");
}

/**
 * 检查并执行分配动作
 * @param currentIndex 当前到达的索引位置
 * @return 1-5表示需要激活的舵机，0表示不需要动作
 */
uint8_t checkAndExecuteAssignment(uint8_t currentIndex) {
    // 检查当前位置是否是出口位置
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        if (currentIndex == divergencePointIndices[i]) {
            // 检查直径值是否有效
            if (carriageDiameters[currentIndex] != INVALID_DIAMETER) {
                // 确定应该分配到哪个出口
                uint8_t outlet = determineOutlet(carriageDiameters[currentIndex]);
                
                // 如果出口匹配当前位置的出口，打开出口
                if (outlet == i + 1) { // 数组索引从0开始，出口编号从1开始
                    Serial.print("在出口位置 ");
                    Serial.print(currentIndex);
                    Serial.print(" 打开出口: 直径 = ");
                    Serial.print(carriageDiameters[currentIndex]);
                    Serial.print(", 出口 = ");
                    Serial.println(outlet);
                    return outlet;
                }
            }
        }
    }
    
    return 0; // 不需要动作
}

/**
 * 重置所有直径数据
 */
void resetAllCarriagesData() {
    for (uint8_t i = 0; i < TOTAL_CARRIAGES; i++) {
        carriageDiameters[i] = INVALID_DIAMETER;
    }
    Serial.println("所有直径数据已重置");
}

/**
 * 根据直径值确定出口
 * @param diameter 直径值
 * @return 出口编号(1-5)
 */
uint8_t determineOutlet(float diameter) {
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

/**
 * 显示所有有效直径数据和队列状态（调试用）
 */
void displayCarriageQueue() {
    Serial.println("\n[CARRIAGE DEBUG] ========= 队列状态详情 =========");
    Serial.println("[CARRIAGE DEBUG] 有效直径数据列表:");
    
    bool hasValidData = false;
    
    // 显示所有有效直径数据
    for (uint8_t i = 0; i < TOTAL_CARRIAGES; i++) {
        if (carriageDiameters[i] != INVALID_DIAMETER) {
            hasValidData = true;
            Serial.print("[CARRIAGE DEBUG] - 位置 ");
            Serial.print(i);
            Serial.print(": 直径 = ");
            Serial.print(carriageDiameters[i]);
            Serial.print("mm");
            
            // 显示该位置对应的预期出口
            uint8_t expectedOutlet = determineOutlet(carriageDiameters[i]);
            Serial.print(", 预期出口: ");
            Serial.print(expectedOutlet);
            
            Serial.println();
        }
    }
    
    if (!hasValidData) {
        Serial.println("[CARRIAGE DEBUG] - 当前没有有效直径数据");
    }
    
    // 显示出口位置信息
    Serial.println("\n[CARRIAGE DEBUG] 出口位置:");
    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        Serial.print("[CARRIAGE DEBUG] - 出口 ");
        Serial.print(i + 1);
        Serial.print(": 位置 ");
        Serial.println(divergencePointIndices[i]);
    }
    
    Serial.println("[CARRIAGE DEBUG] ========= 队列状态结束 =========\n");
}