#include <Arduino.h>
#include "encoder_module.h"

// 定义日志标签
#define TAG "HELLO_WORLD"

// 角度触发回调函数示例
void angleReachedCallback() {
  Serial.println("[角度触发] 达到目标角度！执行特定任务...");
  // 在这里可以添加需要执行的任务代码
  // 例如：控制电机、触发传感器读取、发送信号等
}

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  
  // 等待串口初始化完成
  while (!Serial) {
    ; // 对于Leonardo/Micro/Zero等板，需要等待串口连接
  }
  
  // 打印Hello World信息
  Serial.println("ESP32 Sorter 项目初始化");
  Serial.printf("[%s] ESP32 Chip ID: %08X\n", TAG, ESP.getEfuseMac());
  Serial.printf("[%s] CPU frequency: %d MHz\n", TAG, getCpuFrequencyMhz());
  Serial.printf("[%s] Flash size: %u MB\n", TAG, ESP.getFlashChipSize() / (1024 * 1024));
  
  // 初始化编码器模块
  encoder.initialize();
  
  // 设置角度触发点（例如90度）
  encoder.setAngleTrigger(90, angleReachedCallback);
  
  Serial.println("系统初始化完成，开始运行...");
}

void loop() {
  // 检查是否需要执行角度触发回调
  static bool lastTriggerState = false;
  bool currentTriggerState = encoder.getTriggerState();
  
  if (currentTriggerState && !lastTriggerState) {
    // 触发状态从false变为true，表示刚达到目标角度
    angleReachedCallback();
  }
  lastTriggerState = currentTriggerState;
  
  // 每秒打印一次编码器状态
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastPrintTime >= 1000) {
    lastPrintTime = currentTime;
    
    // 获取编码器信息
    int32_t totalPulses = encoder.getPulseCount();
    int32_t revolutions = encoder.getRevolutionCount();
    uint16_t currentAngle = encoder.getCurrentAngle();
    uint16_t pulsesInRev = encoder.getPulseInCurrentRevolution();
    
    // 打印编码器状态
    Serial.printf("[%s] 编码器状态 - 时间: %lu ms\n", TAG, currentTime);
    Serial.printf("[%s]   总脉冲数: %ld\n", TAG, totalPulses);
    Serial.printf("[%s]   当前圈数: %ld\n", TAG, revolutions);
    Serial.printf("[%s]   圈内脉冲: %u/%u\n", TAG, pulsesInRev, ENCODER_PPR - 1);
    Serial.printf("[%s]   当前角度: %u度\n", TAG, currentAngle);
  }
}