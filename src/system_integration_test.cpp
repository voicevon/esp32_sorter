#include <Arduino.h>
#include "carriage_system.h"
#include "diverter_controller.h"
#include "sorter_controller.h"

/**
 * 系统集成测试函数
 * 此函数用于测试整个传输线待分配托架系统的功能
 * 包括单点扫描仪数据插入、托架移动、直径分类和舵机控制
 */
void runSystemIntegrationTest() {
    Serial.println("\n========== 开始系统集成测试 ==========");
    
    // 测试Carriage类
    Serial.println("\n测试Carriage类:");
    Carriage testCarriage(0);
    testCarriage.setDiameter(12.5);
    Serial.print("  托架索引: ");
    Serial.println(testCarriage.getIndex());
    Serial.print("  直径值: ");
    Serial.println(testCarriage.getDiameter());
    Serial.print("  有效状态: ");
    Serial.println(testCarriage.getValid() ? "有效" : "无效");
    
    // 测试CarriageManager类
    Serial.println("\n测试CarriageManager类:");
    CarriageManager carriageManager;
    
    // 设置分支点位置
    uint8_t testDivergencePoints[NUM_DIVERTERS] = {5, 10, 15, 20, 25};
    carriageManager.setDivergencePoints(testDivergencePoints);
    
    // 添加测试数据
    float testDiameters[] = {18.0, 13.5, 10.0, 7.5, 4.0};
    for (int i = 0; i < sizeof(testDiameters) / sizeof(float); i++) {
        carriageManager.addNewDiameterData(testDiameters[i]);
        // 移动位置，模拟传输
        if (i < sizeof(testDiameters) / sizeof(float) - 1) {
            carriageManager.moveCarriages();
        }
    }
    
    // 检查直径分类
    Serial.println("\n测试直径分类逻辑:");
    for (int i = 0; i < sizeof(testDiameters) / sizeof(float); i++) {
        float diameter = testDiameters[i];
        uint8_t outlet = carriageManager.determineOutlet(diameter);
        Serial.print("  直径 ");
        Serial.print(diameter);
        Serial.print("mm -> 出口");
        Serial.println(outlet);
    }
    
    // 测试分支点分配逻辑
    Serial.println("\n测试分支点分配逻辑:");
    for (int i = 0; i < NUM_DIVERTERS; i++) {
        uint8_t position = testDivergencePoints[i];
        uint8_t diverter = carriageManager.checkAndExecuteAssignment(position);
        Serial.print("  位置 ");
        Serial.print(position);
        Serial.print(": 需要激活分支器 ");
        Serial.println(diverter);
    }
    
    // 重置测试
    Serial.println("\n重置所有测试对象:");
    carriageManager.resetAllCarriages();
    testCarriage.reset();
    
    Serial.println("\n========== 系统集成测试完成 ==========");
}

/**
 * 模拟单点扫描仪功能测试
 * 测试单点扫描仪数据插入到索引0的功能
 */
void testSinglePointScannerSimulation() {
    Serial.println("\n========== 单点扫描仪功能模拟测试 ==========");
    
    CarriageManager carriageManager;
    
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
        carriageManager.addNewDiameterData(currentDiameter);
        
        // 打印当前索引0的托架信息
        Carriage& carriage = carriageManager.getCarriage(0);
        Serial.print("  索引0位置: 直径 = ");
        Serial.print(carriage.getDiameter());
        Serial.print(", 有效 = ");
        Serial.println(carriage.getValid() ? "是" : "否");
        
        // 小延迟模拟时间间隔
        delay(100);
    }
    
    // 测试数据在传输线上移动后的位置
    Serial.println("\n测试数据在传输线上的移动:");
    for (int pos = 0; pos <= 5; pos++) {
        Carriage& carriage = carriageManager.getCarriage(pos);
        if (carriage.getValid()) {
            Serial.print("  位置 ");
            Serial.print(pos);
            Serial.print(": 直径 = ");
            Serial.println(carriage.getDiameter());
        }
    }
    
    Serial.println("\n========== 单点扫描仪功能模拟测试完成 ==========");
}