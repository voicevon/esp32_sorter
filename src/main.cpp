#include <Arduino.h>
#include "carriage_system.h"
#include "outlet_controller.h"
#include "sorter_controller.h"
#include "system_integration_test.h"
#include "pins.h"
#include "diagnostic_controller.h"
#include "debug_module.h"
#include "encoder.h"

// 创建调试模块实例
DebugModule debugModule;

// 创建编码器实例
Encoder encoder;

// 创建分拣控制器实例
SorterController sorterController;

// 创建诊断器实例
DiagnosticController diagnosticController;

// 不需要额外的函数声明，所有功能已封装在诊断器类中

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("ESP32 Sorter system starting...");
  
  // 初始化调试模块
  debugModule.initialize();
  
  // 初始化诊断器
  diagnosticController.initialize(debugModule);
  
  // 等待串口连接
  delay(2000);
  
  // 初始化编码器
  encoder.initialize();
  encoder.enableDebug(true);
  
  // 注册编码器计数值变化回调，将脉冲传递给分拣控制器
  encoder.registerTickCallback([]() {
    // 当编码器计数变化时，通知分拣控制器
    sorterController.handleEncoderPulses(1);
  });
  
  // 初始化分拣控制器
  sorterController.initialize(SERVO_PINS);
  
  // 运行自检
  sorterController.runSelfTest();
  
  // 运行系统集成测试 (仅在调试模式下启用)
  #ifdef DEBUG_MODE
  runSystemIntegrationTest();
  testSinglePointScannerSimulation();
  #endif
  
  // 启动系统
  sorterController.start();
  
  Serial.println("System ready");
  Serial.println("当前模式: " + diagnosticController.getCurrentModeName());
  Serial.println("使用模式按钮切换不同的调试/测试模式");
}

void loop() {
  // 更新诊断器状态
  diagnosticController.update();
  
  // 检查是否有待处理的模式切换
  if (diagnosticController.isModeChangePending()) {
    // 应用模式切换
    diagnosticController.applyModeChange();
  }
  
  // 系统更新
  sorterController.update();
  
  // 处理编码器状态
  if (encoder.isReverseRotation()) {
    Serial.println("[WARNING] Reverse rotation detected! System may need reset.");
  }
  
  // 通过诊断器处理当前工作模式
  diagnosticController.processCurrentMode(sorterController);
  
  // 系统控制通过诊断控制器按钮和硬件接口实现
  
  delay(50); // 小延迟确保系统稳定运行
}