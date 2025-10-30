#include "diverter_controller.h"

DiverterController::DiverterController() {
    initialized = false;
    for (int i = 0; i < NUM_DIVERTERS; i++) {
        servoPins[i] = -1;
        activeDiverters[i] = false;
        actionTimers[i] = 0;
    }
}

void DiverterController::initialize(const int pins[NUM_DIVERTERS]) {
    // 检查是否已初始化
    if (initialized) {
        Serial.println("舵机控制器已经初始化");
        return;
    }

    // 初始化所有舵机
    for (int i = 0; i < NUM_DIVERTERS; i++) {
        if (pins[i] >= 0) {
            servoPins[i] = pins[i];
            diverters[i].attach(pins[i]);
            diverters[i].write(SERVO_STANDARD_POSITION); // 设置到标准位置
            Serial.print("舵机 ");
            Serial.print(i + 1);
            Serial.print(" 初始化在引脚 ");
            Serial.println(pins[i]);
        }
    }

    initialized = true;
    Serial.println("所有舵机初始化完成");
}

void DiverterController::activateDiverter(uint8_t diverterIndex) {
    // 检查索引有效性
    if (diverterIndex < 1 || diverterIndex > NUM_DIVERTERS || !initialized) {
        Serial.println("无效的分支器索引或未初始化");
        return;
    }

    int index = diverterIndex - 1; // 转换为0-4的索引

    // 设置舵机到分配位置
    if (servoPins[index] >= 0) {
        diverters[index].write(SERVO_ASSIGN_POSITION_1);
        activeDiverters[index] = true;
        actionTimers[index] = millis();
        
        Serial.print("激活分支器 ");
        Serial.println(diverterIndex);
    }
}

void DiverterController::resetAllDiverters() {
    if (!initialized) {
        Serial.println("舵机控制器未初始化");
        return;
    }

    for (int i = 0; i < NUM_DIVERTERS; i++) {
        if (servoPins[i] >= 0) {
            diverters[i].write(SERVO_STANDARD_POSITION);
            activeDiverters[i] = false;
        }
    }
    
    Serial.println("所有分支器已重置到标准位置");
}

void DiverterController::resetDiverter(uint8_t diverterIndex) {
    // 检查索引有效性
    if (diverterIndex < 1 || diverterIndex > NUM_DIVERTERS || !initialized) {
        Serial.println("无效的分支器索引或未初始化");
        return;
    }

    int index = diverterIndex - 1; // 转换为0-4的索引

    if (servoPins[index] >= 0) {
        diverters[index].write(SERVO_STANDARD_POSITION);
        activeDiverters[index] = false;
        
        Serial.print("分支器 ");
        Serial.print(diverterIndex);
        Serial.println(" 已重置到标准位置");
    }
}

void DiverterController::update() {
    if (!initialized) {
        return;
    }

    // 检查每个活跃的分支器是否需要重置
    unsigned long currentTime = millis();
    for (int i = 0; i < NUM_DIVERTERS; i++) {
        if (activeDiverters[i]) {
            if (currentTime - actionTimers[i] >= SERVO_RECOVERY_DELAY) {
                resetDiverter(i + 1);
            }
        }
    }
}

void DiverterController::testAllDiverters() {
    if (!initialized) {
        Serial.println("舵机控制器未初始化，无法测试");
        return;
    }

    Serial.println("开始测试所有分支器...");
    
    // 测试每个分支器
    for (int i = 0; i < NUM_DIVERTERS; i++) {
        if (servoPins[i] >= 0) {
            Serial.print("测试分支器 ");
            Serial.println(i + 1);
            
            // 移动到分配位置
            diverters[i].write(SERVO_ASSIGN_POSITION_1);
            delay(SERVO_ACTION_DELAY);
            
            // 移动到另一个位置（如果支持）
            diverters[i].write(SERVO_ASSIGN_POSITION_2);
            delay(SERVO_ACTION_DELAY);
            
            // 回到标准位置
            diverters[i].write(SERVO_STANDARD_POSITION);
            delay(SERVO_RECOVERY_DELAY);
        }
    }
    
    Serial.println("分支器测试完成");
}

void DiverterController::setDiverterPosition(uint8_t diverterIndex, int angle) {
    // 检查索引有效性
    if (diverterIndex < 1 || diverterIndex > NUM_DIVERTERS || !initialized) {
        Serial.println("无效的分支器索引或未初始化");
        return;
    }

    int index = diverterIndex - 1; // 转换为0-4的索引

    // 限制角度范围
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    if (servoPins[index] >= 0) {
        diverters[index].write(angle);
        
        Serial.print("设置分支器 ");
        Serial.print(diverterIndex);
        Serial.print(" 到角度 ");
        Serial.println(angle);
    }
}