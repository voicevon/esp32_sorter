#include "outlet_controller.h"

/**
 * 出口控制器构造函数
 * 初始化所有出口状态变量
 */
OutletController::OutletController() {
    initialized = false;
    for (int i = 0; i < NUM_OUTLETS; i++) {
        servoPins[i] = -1;
        activeOutlets[i] = false;
        actionTimers[i] = 0;
    }
}

/**
 * 初始化所有出口的舵机
 * @param pins 舵机引脚数组，对应5个固定出口
 */
void OutletController::initialize(const int pins[NUM_OUTLETS]) {
    // 检查是否已初始化
    if (initialized) {
        Serial.println("出口控制器已经初始化");
        return;
    }

    // 初始化所有出口的舵机
    for (int i = 0; i < NUM_OUTLETS; i++) {
        if (pins[i] >= 0) {
            servoPins[i] = pins[i];
            outlets[i].attach(pins[i]);
            outlets[i].write(SERVO_CLOSED_POSITION); // 设置到关闭位置
            Serial.print("出口 ");
            Serial.print(i + 1);
            Serial.print(" 舵机初始化在引脚 ");
            Serial.println(pins[i]);
        }
    }

    initialized = true;
    Serial.println("所有出口舵机初始化完成");
}

/**
 * 打开指定的出口
 * @param outletIndex 出口索引（1-5），对应5个固定出口
 */
void OutletController::openOutlet(uint8_t outletIndex) {
    // 检查索引有效性
    if (outletIndex < 1 || outletIndex > NUM_OUTLETS || !initialized) {
        Serial.println("无效的出口索引或未初始化");
        return;
    }

    int index = outletIndex - 1; // 转换为0-4的索引

    // 设置出口的舵机到打开位置
    if (servoPins[index] >= 0) {
        outlets[index].write(SERVO_OPEN_POSITION_1);
        activeOutlets[index] = true;
        actionTimers[index] = millis();
        
        Serial.print("打开出口 ");
        Serial.println(outletIndex);
    }
}

/**
 * 关闭所有出口
 */
void OutletController::closeAllOutlets() {
    if (!initialized) {
        Serial.println("出口控制器未初始化");
        return;
    }

    // 关闭所有出口
    for (int i = 0; i < NUM_OUTLETS; i++) {
        if (servoPins[i] >= 0) {
            outlets[i].write(SERVO_CLOSED_POSITION);
            activeOutlets[i] = false;
        }
    }
    
    Serial.println("所有出口已关闭");
}

/**
 * 关闭单个出口
 * @param outletIndex 出口索引（1-5），对应5个固定出口
 */
void OutletController::closeOutlet(uint8_t outletIndex) {
    // 检查索引有效性
    if (outletIndex < 1 || outletIndex > NUM_OUTLETS || !initialized) {
        Serial.println("无效的出口索引或未初始化");
        return;
    }

    int index = outletIndex - 1; // 转换为0-4的索引

    if (servoPins[index] >= 0) {
        outlets[index].write(SERVO_CLOSED_POSITION);
        activeOutlets[index] = false;
        
        Serial.print("出口 ");
        Serial.print(outletIndex);
        Serial.println(" 已关闭");
    }
}

/**
 * 更新出口的控制逻辑
 * 应在主循环中调用，主要用于维护状态
 * 注意：出口的关闭现在由SorterController基于编码器位置触发，不再通过定时器自动关闭
 */
void OutletController::update() {
    if (!initialized) {
        return;
    }
    
    // 现在update方法仅保留基本的状态维护功能
    // 出口的关闭将由SorterController在适当的编码器位置触发
}

/**
 * 测试所有出口的舵机功能
 * 依次测试每个出口的动作
 */
void OutletController::testAllOutlets() {
    if (!initialized) {
        Serial.println("出口控制器未初始化，无法测试");
        return;
    }

    Serial.println("开始测试所有出口...");
    
    // 测试每个出口
    for (int i = 0; i < NUM_OUTLETS; i++) {
        if (servoPins[i] >= 0) {
            Serial.print("测试出口 ");
            Serial.println(i + 1);
            
            // 移动到打开位置1
            outlets[i].write(SERVO_OPEN_POSITION_1);
            delay(SERVO_ACTION_DELAY);
            
            // 移动到打开位置2（如果支持）
            outlets[i].write(SERVO_OPEN_POSITION_2);
            delay(SERVO_ACTION_DELAY);
            
            // 回到关闭位置
            outlets[i].write(SERVO_CLOSED_POSITION);
            delay(SERVO_RECOVERY_DELAY);
        }
    }
    
    Serial.println("出口测试完成");
}


// 此处之前是setDiverterPosition方法，已移除