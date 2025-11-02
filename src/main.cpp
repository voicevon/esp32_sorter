#include <Arduino.h>
#include "pins.h"



// 始终包含必要的头文件
#include "outlet.h"


// 正常工作模式所需的头文件
#include "carriage_system.h"
#include "system_integration_test.h"
#include "simple_hmi.h"
#include "encoder.h"
#include "sorter.h"

// 常量定义
#define NUM_OUTLETS 5

// 系统工作模式定义
enum SystemMode {
  MODE_NORMAL = 0,            // 正常工作模式
  MODE_DIAGNOSE_ENCODER = 1,  // 诊断编码器模式
  MODE_DIAGNOSE_SCANNER = 2,  // 诊断扫描仪模式
  MODE_DIAGNOSE_OUTLET = 3,   // 诊断出口模式
  MODE_DIAGNOSE_CONVEYOR = 4, // 诊断传输线模式
  MODE_TEST = 5               // 测试模式
};

// 全局变量定义

// 模式相关变量
SystemMode currentMode = MODE_NORMAL;  // 当前模式
SystemMode pendingMode = MODE_NORMAL;  // 待切换模式
bool modeChangePending = false;        // 模式切换标志

// 时间相关变量
unsigned long lastMoveTime = 0;        // 上次移动时间
unsigned long lastDataTime = 0;        // 上次数据生成时间

// 创建人机交互模块实例
SimpleHMI simpleHMI;

// 创建编码器实例
Encoder encoder;

// 创建Sorter实例
Sorter sorter;

// 函数声明
String getCurrentModeName();









void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("ESP32 Sorter system starting...");
  
  // 初始化人机交互模块
  simpleHMI.initialize();
  
  // 等待串口连接
  delay(2000);
  
  // 初始化编码器
  encoder.initialize();
  encoder.enableDebug(true);
  
  // 初始化Sorter
  sorter.initialize();
  
  Serial.println("System ready");
  Serial.println("当前模式: " + getCurrentModeName());
  Serial.println("使用模式按钮切换不同的调试/测试模式");
}



void loop() {
  // 更新人机交互模块状态
  simpleHMI.spin_once();
  
  // 检查主按钮是否被按下（模式切换）
  if (simpleHMI.isButtonPressed(MASTER_BUTTON)) {
    // 切换到下一个工作模式
    pendingMode = static_cast<SystemMode>((currentMode + 1) % 6); // 6种模式循环切换
    modeChangePending = true;
    
    // 打印模式切换请求信息
    Serial.print("[DIAGNOSTIC] Mode switch requested to: ");
    Serial.println(getCurrentModeName());
    simpleHMI.clearButtonStates();
  }
  
  // 检查是否有待处理的模式切换
  if (modeChangePending) {
    // 应用模式切换
    currentMode = pendingMode;
    modeChangePending = false;
    
    // 在外部控制LED状态以反映当前模式
    if (currentMode == MODE_NORMAL) {
      simpleHMI.setLEDState(MASTER_LED, false);
      simpleHMI.setLEDState(SLAVE_LED, false);
    } else if (currentMode == MODE_DIAGNOSE_ENCODER) {
      simpleHMI.setLEDState(MASTER_LED, true);
      simpleHMI.setLEDState(SLAVE_LED, false);
    } else if (currentMode == MODE_DIAGNOSE_SCANNER) {
      simpleHMI.setLEDState(MASTER_LED, false);
      simpleHMI.setLEDState(SLAVE_LED, true);
    } else {
      simpleHMI.setLEDState(MASTER_LED, true);
      simpleHMI.setLEDState(SLAVE_LED, true);
    }
    
    // 打印模式切换完成信息
    Serial.print("[DIAGNOSTIC] Mode switched to: ");
    Serial.println(getCurrentModeName());
  }
  
  // 在正常模式下调用Sorter的spin_Once()函数
  if (currentMode == MODE_NORMAL) {
    sorter.spin_Once();
  }
  
  // 处理编码器状态
  if (encoder.isReverseRotation()) {
    Serial.println("[WARNING] Reverse rotation detected! System may need reset.");
  }
  
  // 当不在正常模式时，根据不同模式执行相应的日志输出
  if (currentMode != MODE_NORMAL) {
    static unsigned long lastLogTime = 0;
    unsigned long currentTime = millis();
    
    // 每秒输出一次模式信息，避免日志过于频繁
    if (currentTime - lastLogTime > 1000) {
      lastLogTime = currentTime;
      
      switch (currentMode) {
        case MODE_DIAGNOSE_ENCODER:
          Serial.println("[DIAGNOSTIC] Encoder diagnostic mode");
          break;
        case MODE_DIAGNOSE_SCANNER:
          Serial.println("[DIAGNOSTIC] Scanner diagnostic mode");
          break;
        case MODE_DIAGNOSE_OUTLET:
          Serial.println("[DIAGNOSTIC] Outlet diagnostic mode");
          break;
        case MODE_DIAGNOSE_CONVEYOR:
          Serial.println("[DIAGNOSTIC] Conveyor diagnostic mode");
          break;
        case MODE_TEST:
          Serial.println("[TEST] Test mode");
          break;
        default:
          break;
      }
    }
  }
  
  // 处理当前工作模式
  unsigned long currentTime = millis();
  
  switch (currentMode) {
    case MODE_DIAGNOSE_ENCODER:
      // 诊断编码器模式
      Serial.println("[DIAGNOSTIC] Encoder diagnostic mode - reading real encoder values");
      break;
    
    case MODE_DIAGNOSE_SCANNER:
      // 诊断扫描仪模式
      Serial.println("[DIAGNOSTIC] Scanner diagnostic mode");
      break;
    
    case MODE_DIAGNOSE_OUTLET:
      // 诊断出口模式已在diagnosticLoop中处理
      break;
    
    case MODE_DIAGNOSE_CONVEYOR:
      // 诊断传输线模式
      Serial.println("[DIAGNOSTIC] Conveyor diagnostic mode");
      displayCarriageQueue();
      break;
    
    case MODE_TEST:
      // 测试模式
      Serial.println("[TEST] Test mode");
      break;
    
    case MODE_NORMAL:
    default:
      // 正常模式下的处理已移至主循环中的Sorter::spin_Once()调用
      break;
  }
}

// 函数实现

String getCurrentModeName() {
  // 获取当前模式名称
  switch (currentMode) {
    case MODE_NORMAL:
      return "正常模式";
    case MODE_DIAGNOSE_ENCODER:
      return "编码器诊断模式";
    case MODE_DIAGNOSE_SCANNER:
      return "扫描仪诊断模式";
    case MODE_DIAGNOSE_OUTLET:
      return "出口诊断模式";
    case MODE_DIAGNOSE_CONVEYOR:
      return "传输线诊断模式";
    case MODE_TEST:
      return "测试模式";
    default:
      return "未知模式";
  }
}