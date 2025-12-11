#include <Arduino.h>
#include "pins.h"

// 必要的头文件
#include "outlet.h"
#include "simple_hmi.h"
#include "encoder.h"
#include "sorter.h"
// #include "oled.h"  // 屏蔽OLED头文件

// 引入系统工作模式定义
#include "main.h"

// 全局变量定义

// 模式相关变量
SystemMode currentMode = MODE_NORMAL;  // 当前模式（修改为出口测试模式）
SystemMode pendingMode = MODE_NORMAL;  // 待切换模式（与当前模式保持一致）
bool modeChangePending = false;        // 模式切换标志

// 使用单例模式获取人机交互模块实例
SimpleHMI* simpleHMI = SimpleHMI::getInstance();

// 使用单例模式获取Encoder实例
Encoder* encoder = Encoder::getInstance();

// 使用单例模式获取OLED实例
// OLED* oled = OLED::getInstance();  // 屏蔽OLED实例

// 创建Sorter实例
Sorter sorter;

// 函数声明
String getCurrentModeName();









void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("ESP32 Sorter system starting...");
  
  // 初始化人机交互模块
  simpleHMI->initialize();
  
  // 串口初始化完成后无需等待连接建立
  
  // 初始化编码器
  encoder->initialize();
  
  // 出口位置初始化已移至Sorter类的initialize方法中
  
  // 初始化Sorter
  sorter.initialize();
  
  // 初始化OLED显示器
  // oled->initialize();  // 屏蔽OLED初始化
  
  Serial.println("System ready");
  Serial.println("当前模式: " + getCurrentModeName());
  Serial.println("使用模式按钮切换不同的调试/测试模式");
}



void loop() {
  // 检查主按钮是否被按下（模式切换）
  if (simpleHMI->isMasterButtonPressed()) {
    // 切换到下一个工作模式
    pendingMode = static_cast<SystemMode>((currentMode + 1) % 7); // 7种模式循环切换
    modeChangePending = true;
    
    // 打印模式切换请求信息
    Serial.print("[DIAGNOSTIC] Mode switch requested to: ");
    // 临时保存当前模式，显示待切换的模式名称
    SystemMode tempMode = currentMode;
    currentMode = pendingMode;
    Serial.println(getCurrentModeName());
    currentMode = tempMode; // 恢复原模式
    // 注意：isMasterButtonPressed()方法会自动清除标志
  }
  
  // 检查从按钮是否被按下（可用于其他功能扩展）
  if (simpleHMI->isSlaveButtonPressed()) {
    // 从按钮功能处理（当前未使用，可根据需要扩展）
    Serial.println("[DIAGNOSTIC] Slave button pressed");
    // 注意：isSlaveButtonPressed()方法会自动清除标志
  }
  
  // 检查是否有待处理的模式切换
  if (modeChangePending) {
    // 应用模式切换
    currentMode = pendingMode;
    modeChangePending = false;
    
    // 移除模式LED控制，LED现在只用于显示出口状态
    
    // 打印模式切换完成信息
      Serial.print("[DIAGNOSTIC] Mode switched to: ");
      Serial.println(getCurrentModeName());
    
    // 显示模式变化信息到OLED
    // oled->displayModeChange(currentMode);  // 屏蔽OLED模式显示
  }
  

  
  // 处理当前工作模式
  unsigned long currentTime = millis();
  
  switch (currentMode) {
    case MODE_DIAGNOSE_ENCODER:
      // 诊断编码器模式
      encoder->printout();
      break;
    
    case MODE_DIAGNOSE_SCANNER: {
      // 诊断扫描仪模式 - 读取并打印扫描仪状态（仅状态变化时输出）
      static bool lastScannerState = false;
      int currentState = digitalRead(LASER_SCANNER_PIN);
      
      // 只在状态发生变化时输出日志
      if (currentState != lastScannerState) {
        lastScannerState = currentState;
        Serial.print("[DIAGNOSTIC] Scanner state changed to: ");
        Serial.println(currentState ? "HIGH" : "LOW");
        

      }
      break;
    }
    
    case MODE_DIAGNOSE_OUTLET: {
      // 诊断出口模式 - 依次测试所有出口并循环
      static unsigned long modeStartTime = 0;
      static unsigned long lastOutletTime = 0;
      static bool outletState = false;
      static uint8_t currentOutlet = 0;
      static uint8_t MAX_OUTLETS = 0;
      
      // 模式开始时初始化
      if (modeStartTime == 0) {
        modeStartTime = currentTime;
        lastOutletTime = currentTime;
        outletState = false;
        currentOutlet = 0;
        // 获取出口数量
        MAX_OUTLETS = sorter.getOutletCount();
        Serial.println("[DIAGNOSTIC] 诊断出口模式已启动 - 依次测试所有出口（0-" + String(MAX_OUTLETS - 1) + "）");
      }
      
      // 每5秒切换一次状态
      if (currentTime - lastOutletTime >= 5000) {
        lastOutletTime = currentTime;
        outletState = !outletState;
        
        // 当状态从关闭切换到打开时，移动到下一个出口
        if (outletState) {
          currentOutlet = (currentOutlet + 1) % MAX_OUTLETS;
          Serial.print("[DIAGNOSTIC] 现在测试出口: ");
          Serial.println(currentOutlet);
        }
        
        // 使用公共方法控制当前出口
        sorter.setOutletState(currentOutlet, outletState);
      }
      break;
    }
    
    case MODE_DIAGNOSE_CONVEYOR:
      // 诊断传输线模式
      break;
    
    case MODE_TEST:
      // 测试模式
      break;
    
    case MODE_TEST_RELOADER: {
      // 上料器测试模式（Feeder Test Mode）
      static unsigned long modeStartTime = 0;
      static unsigned long lastReloaderTime = 0;
      static bool reloaderState = false;
      
      // 模式开始时初始化
      if (modeStartTime == 0) {
        modeStartTime = currentTime;
        lastReloaderTime = currentTime;
        reloaderState = false;
        Serial.println("[DIAGNOSTIC] 上料器测试模式（Feeder Test Mode）已启动");
      }
      
      // 计算模式运行时间
      unsigned long modeRunTime = currentTime - modeStartTime;
      
      // 前15秒，每5秒开关一次
      if (modeRunTime <= 15000) {
        if (currentTime - lastReloaderTime >= 5000) {
          lastReloaderTime = currentTime;
          reloaderState = !reloaderState;
          if (reloaderState) {
            sorter.openReloader();  // 使用公共方法开启上料器
          } else {
            sorter.closeReloader(); // 使用公共方法关闭上料器
          }
        }
      } else {
        // 15秒后，关闭并保持5秒
        if (reloaderState) {
          sorter.closeReloader();   // 使用公共方法确保关闭
          reloaderState = false;
          Serial.println("[DIAGNOSTIC] 15秒后，上料器已关闭并保持");
        }
      }
      break;
    }
    
    case MODE_NORMAL:
    default:
      // 正常模式下，系统通过编码器相位变化触发Sorter的onPhaseChange方法
      break;
  }
  
  // 处理编码器触发的任务（在所有模式下都执行）
  sorter.spinOnce();
  
  // 更新OLED显示内容
  // oled->update(currentMode, sorter.getOutletCount());  // 屏蔽OLED更新
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
    case MODE_TEST_RELOADER:
      return "上料器测试模式（Feeder Test Mode）";
    default:
      return "未知模式";
  }
}