#include <Arduino.h>
#include "tray_system.h"

/**
 * 运行系统集成测试
 */
void runSystemIntegrationTest() {
    Serial.println("\n========== 开始系统集成测试 ==========");
    
    // 创建托盘系统
    TraySystem traySystem;
    
    // 初始化出口位置
    Serial.println("\n初始化出口位置:");
    uint8_t testOutletPositions[5] = {5, 10, 15, 20, 25};
    
    // 添加测试数据
    Serial.println("\n添加测试数据:");
    for (int i = 0; i < 5; i++) {
        int diameter = 5 + i * 2; // 5, 7, 9, 11, 13 mm
        int scanCount = 50 + i * 10; // 50, 60, 70, 80, 90 次扫描
        traySystem.addNewDiameterData(diameter, scanCount);
        Serial.printf("  添加直径: %d mm, 扫描次数: %d\n", diameter, scanCount);
    }
    
    // 显示当前托盘状态
    traySystem.displayTrayQueue();
    
    // 测试移动托盘数据
    Serial.println("\n测试移动托盘数据:");
    for (int i = 0; i < 3; i++) {
        traySystem.moveTraysData();
        Serial.printf("  移动后托盘队列状态 (第%d次):\n", i + 1);
        traySystem.displayTrayQueue();
    }
    
    // 测试重置功能
    Serial.println("\n测试重置功能:");
    traySystem.resetAllTraysData();
    traySystem.displayTrayQueue();
    
    Serial.println("\n========== 系统集成测试完成 ==========");
}

/**
 * 模拟单点扫描仪功能测试
 * 测试单点扫描仪数据插入到索引0的功能
 */
void testSinglePointScannerSimulation() {
    Serial.println("\n========== 单点扫描仪功能模拟测试 ==========");
    
    // 创建CarriageSystem实例
    TraySystem traySystem;
    
    // 重置数据
    traySystem.resetAllTraysData();
    
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
        traySystem.addNewDiameterData(currentDiameter, 100);  // 假设扫描次数为100
        
        // 打印当前索引0的直径信息
        Serial.print("  索引0位置: 直径 = ");
        Serial.print(traySystem.getTrayDiameter(0));
        Serial.print("  有效 = ");
        Serial.println(traySystem.getTrayDiameter(0) != -1 ? "是" : "否");
        
        // 小延迟模拟时间间隔
        delay(100);
    }
    
    // 显示当前队列状态
    traySystem.displayTrayQueue();
    
    Serial.println("\n========== 单点扫描仪功能模拟测试完成 ==========");
}