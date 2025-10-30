#include <Arduino.h>
#include "carriage_system.h"
#include "diverter_controller.h"
#include "sorter_controller.h"
#include "system_integration_test.h"
#include "pins.h"
#include "diagnostic_controller.h"

// 创建分拣控制器实例
SorterController sorterController;

// 创建诊断器实例
DiagnosticController diagnosticController;

// 不需要额外的函数声明，所有功能已封装在诊断器类中

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("ESP32 Sorter 系统启动中...");
  
  // 初始化诊断器
  diagnosticController.initialize();
  
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
  Serial.println("当前模式: " + diagnosticController.getCurrentModeName());
  Serial.println("使用命令 T 或模式按钮切换不同的调试/测试模式");
}

void loop() {
  // 更新诊断器状态
  diagnosticController.update();
  
  // 检查是否有待处理的模式切换
  if (diagnosticController.isModeChangePending()) {
    // 在循环结束前应用模式切换
    // 注意：这里在系统更新前应用，确保下一次循环使用新模式
    diagnosticController.applyModeChange();
  }
  
  // 系统更新
  sorterController.update();
  
  // 根据当前系统模式执行不同的操作
  switch (diagnosticController.getCurrentMode()) {
    case MODE_TEST:
      // 测试模式：定期生成模拟直径数据和移动传输线
      {
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
      break;
      
    case MODE_DEBUG_SCANNER:
      // 扫描仪调试模式：只生成模拟直径数据
      {
        static unsigned long lastDataTime = 0;
        const unsigned long DATA_INTERVAL = 2000; // 2秒生成一次数据
        
        if (millis() - lastDataTime >= DATA_INTERVAL) {
          lastDataTime = millis();
          
          // 生成随机直径 (3mm - 20mm)
          float randomDiameter = 3.0 + random(0, 171) * 0.1;
          
          Serial.print("[扫描仪调试] 生成直径数据: ");
          Serial.println(randomDiameter);
          
          sorterController.receiveDiameterData(randomDiameter);
        }
      }
      break;
      
    case MODE_DEBUG_ENCODER:
      // 编码器调试模式：只模拟移动传输线
      {
        static unsigned long lastMoveTime = 0;
        const unsigned long MOVE_INTERVAL = 1500; // 1.5秒移动一次
        
        if (millis() - lastMoveTime >= MOVE_INTERVAL) {
          lastMoveTime = millis();
          
          Serial.println("[编码器调试] 移动一个位置");
          sorterController.moveOnePosition();
          
          // 显示当前位置信息
          Serial.print("当前位置: ");
          Serial.println(sorterController.getCurrentPosition());
        }
      }
      break;
      
    case MODE_DEBUG_DIVERTER:
      // 分支器调试模式：可以通过命令单独控制分支器
      break;
      
    case MODE_NORMAL:
    default:
      // 正常工作模式：不执行额外操作
      break;
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
        // 通过串口命令切换系统模式
        diagnosticController.switchModeExternally();
        break;
      case '1'...'5':
        sorterController.testDiverter(command - '0');
        break;
      case '?':
        Serial.println("命令帮助：");
        Serial.println("  R - 重置系统");
        Serial.println("  S - 启动系统");
        Serial.println("  P - 停止系统");
        Serial.println("  T - 循环切换系统模式（正常->编码器调试->扫描仪调试->分支器调试->测试->正常）");
        Serial.println("  1-5 - 测试对应分支器");
        Serial.println("  ? - 显示此帮助");
        break;
    }
  }
  
  delay(50); // 小延迟确保系统稳定运行
}