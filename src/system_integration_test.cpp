#include <Arduino.h>
#include "carriage_system.h"

/**
 * 系统集成测试函数
 * 此函数用于测试整个传输线待分配系统的功能
 * 包括单点扫描仪数据插入、数据移动、直径分类和舵机控制
 */
void runSystemIntegrationTest() {
    Serial.println("\n========== 开始系统集成测试 ==========");
    
    // 初始化出口位置
    Serial.println("\n初始化出口位置:");
    uint8_t testOutletPositions[NUM_OUTLETS] = {5, 10, 15, 20, 25};
    initializeDivergencePoints(testOutletPositions);
    
    // 添加测试数据
    float testDiameters[] = {18.0, 13.5, 10.0, 7.5, 4.0};
    for (int i = 0; i < sizeof(testDiameters) / sizeof(float); i++) {
        addNewDiameterData(testDiameters[i]);
        // 移动位置，模拟传输
        if (i < sizeof(testDiameters) / sizeof(float) - 1) {
            moveCarriagesData();
        }
    }
    
    // 显示当前队列状态
    displayCarriageQueue();
    
    // 检查直径分类
    Serial.println("\n测试直径分类逻辑:");
    for (int i = 0; i < sizeof(testDiameters) / sizeof(float); i++) {
        float diameter = testDiameters[i];
        uint8_t outlet = determineOutlet(diameter);
        Serial.print("  直径 ");
        Serial.print(diameter);
        Serial.print("mm -> 出口");
        Serial.println(outlet);
    }
    
    // 测试出口分配逻辑
    Serial.println("\n测试出口分配逻辑:");
    for (int i = 0; i < NUM_OUTLETS; i++) {
        uint8_t position = testOutletPositions[i];
        uint8_t outlet = checkAndExecuteAssignment(position);
        Serial.print("  位置 ");
        Serial.print(position);
        Serial.print(": 需要激活出口 ");
        Serial.println(outlet);
    }
    
    // 重置测试
    Serial.println("\n重置所有测试数据:");
    resetAllCarriagesData();
    
    Serial.println("\n========== 系统集成测试完成 ==========");
}

/**
 * 模拟单点扫描仪功能测试
 * 测试单点扫描仪数据插入到索引0的功能
 */
void testSinglePointScannerSimulation() {
    Serial.println("\n========== 单点扫描仪功能模拟测试 ==========");
    
    // 重置数据
    resetAllCarriagesData();
    
    // 模拟连续扫描多个芦笋
    Serial.println("\n模拟连续扫描6个芦笋:");
    float scanDiameters[] = {16.2, 12.8, 9.5, 7.2, 5.8, 20.5}; // 包含一个异常值
    
    for (int i = 0; i < sizeof(scanDiameters) / sizeof(float); i++) {
        float currentDiameter = scanDiameters[i];
        Serial.print("  扫描到第 ");
        Serial.print(i + 1);
        Serial.print(" 个芦笋，直径: ");
        Serial.print(currentDiameter);
        Serial.println("mm");
        
        // 添加到索引0位置
        addNewDiameterData(currentDiameter);
        
        // 打印当前索引0的直径信息
        Serial.print("  索引0位置: 直径 = ");
        Serial.print(carriageDiameters[0]);
        Serial.print("  有效 = ");
        Serial.println(carriageDiameters[0] != INVALID_DIAMETER ? "是" : "否");
        
        // 小延迟模拟时间间隔
        delay(100);
    }
    
    // 显示当前队列状态
    displayCarriageQueue();
    
    Serial.println("\n========== 单点扫描仪功能模拟测试完成 ==========");
}