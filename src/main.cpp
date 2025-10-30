#include <Arduino.h>
#include "carriage_system.h"
#include "diverter_controller.h"
#include "sorter_controller.h"
#include "system_integration_test.h"

// 舵机引脚定义
const int SERVO_PINS[NUM_DIVERTERS] = {
    2,   // 分支器1舵机
    4,   // 分支器2舵机
    5,   // 分支器3舵机
    18,  // 分支器4舵机
    19   // 分支器5舵机
};

// 创建分拣控制器实例
SorterController sorterController;

// 测试模式标志
bool testMode = true;

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("ESP32 Sorter 系统启动中...");
  
  // 等待串口连接
  delay(2000);
  
  // 初始化分拣控制器
  sorterController.initialize(SERVO_PINS);
  
  // 运行自检
  sorterController.runSelfTest();
  
  // 运行系统集成测试
  runSystemIntegrationTest();
  testSinglePointScannerSimulation();
  
  // 启动系统
  sorterController.start();
  
  Serial.println("系统已准备就绪");
  Serial.println("在测试模式下，将定期生成模拟直径数据并移动传输线");
}

void loop() {
  // 系统更新
  sorterController.update();
  
  if (testMode) {
    // 测试模式：定期生成模拟直径数据
    static unsigned long lastDataTime = 0;
    static unsigned long lastMoveTime = 0;
    const unsigned long DATA_INTERVAL = 3000; // 3秒生成一次数据
    const unsigned long MOVE_INTERVAL = 1000; // 1秒移动一次
    
    // 生成模拟直径数据
    if (millis() - lastDataTime >= DATA_INTERVAL) {
      lastDataTime = millis();
      
      // 生成随机直径 (3mm - 20mm)
      float randomDiameter = 3.0 + random(0, 171) * 0.1; // 3.0到20.0的随机直径
      
      Serial.print("生成模拟直径数据: ");
      Serial.println(randomDiameter);
      
      // 接收直径数据
      sorterController.receiveDiameterData(randomDiameter);
    }
    
    // 模拟移动传输线
    if (millis() - lastMoveTime >= MOVE_INTERVAL) {
      lastMoveTime = millis();
      sorterController.moveOnePosition();
    }
  }
  
  // 检查串口输入（可选：用于调试控制）
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    switch (command) {
      case 'r':
      case 'R':
        sorterController.reset();
        break;
      case 's':
      case 'S':
        sorterController.start();
        break;
      case 'p':
      case 'P':
        sorterController.stop();
        break;
      case 't':
      case 'T':
        testMode = !testMode;
        Serial.print("测试模式：");
        Serial.println(testMode ? "开启" : "关闭");
        break;
      case '1'...'5':
        sorterController.testDiverter(command - '0');
        break;
      case '?':
        Serial.println("命令帮助：");
        Serial.println("  R - 重置系统");
        Serial.println("  S - 启动系统");
        Serial.println("  P - 停止系统");
        Serial.println("  T - 切换测试模式");
        Serial.println("  1-5 - 测试对应分支器");
        Serial.println("  ? - 显示此帮助");
        break;
    }
  }
  
  delay(50); // 小延迟确保系统稳定运行
}