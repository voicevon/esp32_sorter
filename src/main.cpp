#include <Arduino.h>

// #include "outlet.h"
#include "user_interface.h"
#include "encoder.h"
#include "sorter.h"

// 引入系统工作模式定义
#include "main.h"

// 全局变量定义

// 版本信息
String versionNumber = "ver: 2601";

// 版本信息已显示标志
bool versionInfoAlreadyDisplayed = false;

// 正常模式子模式
int normalSubMode = 0;  // 0: 统计信息, 1: 最新直径

// 编码器诊断模式子模式
int encoderSubMode = 0;  // 0: 位置显示, 1: 相位变化

// 出口测试模式子模式
int outletSubMode = 0;  // 0: 轮巡降落（常态打开，偶尔闭合），1: 轮巡上升（常态闭合，偶尔打开）

// 模式相关变量
SystemMode currentMode = MODE_NORMAL;  // 当前模式（修改为出口测试模式）
SystemMode pendingMode = MODE_NORMAL;  // 待切换模式（与当前模式保持一致）
bool modeChangePending = false;        // 模式切换标志

// 使用单例模式获取人机交互模块实例
UserInterface* userInterface = UserInterface::getInstance();

// 使用单例模式获取Encoder实例
Encoder* encoder = Encoder::getInstance();

// 创建Sorter实例
Sorter sorter;

// 函数声明
String getModeName(SystemMode mode);









void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("ESP32 Sorter system starting...");
  
  // 初始化用户界面（包含OLED和SimpleHMI）
  userInterface->initialize();
  
  // 串口初始化完成后无需等待连接建立
  
  // 初始化编码器
  encoder->initialize();
  
  // 出口位置初始化已移至Sorter类的initialize方法中
  
  // 初始化Sorter
  sorter.initialize();
  
  Serial.println("System ready");
  Serial.println("当前模式: " + getModeName(currentMode));
  Serial.println("使用模式按钮切换不同的调试/测试模式");
}



// 处理主按钮（模式切换）
void handleMasterButton() {
  if (userInterface->isMasterButtonPressed()) {
    // 切换到下一个工作模式
    pendingMode = static_cast<SystemMode>((currentMode + 1) % 6); // 6种模式循环切换
    modeChangePending = true;
    
    // 打印模式切换请求信息
    Serial.print("[DIAGNOSTIC] Mode switch requested to: ");
    // 临时保存当前模式，显示待切换的模式名称
    SystemMode tempMode = currentMode;
    currentMode = pendingMode;
    Serial.println(getModeName(currentMode));
    currentMode = tempMode; // 恢复原模式
    // 注意：isMasterButtonPressed()方法会自动清除标志
  }
}

// 处理从按钮（子模式切换）
void handleSlaveButton() {
  if (userInterface->isSlaveButtonPressed()) {
    // 从按钮功能处理
    if (currentMode == MODE_DIAGNOSE_SCANNER) {
      // 在扫描仪诊断模式下，切换子测试模式
      static int scannerSubMode = 0;
      scannerSubMode = (scannerSubMode + 1) % 3;  // 3个子模式循环切换
      
      String subModeName = "";
      switch (scannerSubMode) {
        case 0:
          subModeName = "Scanner IO Status";
          break;
        case 1:
          subModeName = "Diameter Test";
          break;
        case 2:
          subModeName = "Cycle Switch";
          break;
      }
      
      Serial.println("[DIAGNOSTIC] 切换到子模式: " + subModeName);
      userInterface->displayDiagnosticInfo("Scanner Diag", "SubMode: " + subModeName);
    } else if (currentMode == MODE_NORMAL) {
      // 在正常模式下，切换子显示模式
      normalSubMode = (normalSubMode + 1) % 2;  // 2个子模式循环切换
      
      String subModeName = normalSubMode == 0 ? "统计信息" : "最新直径";
      Serial.println("[NORMAL] 切换到子模式: " + subModeName);
      userInterface->displayDiagnosticInfo("Normal Mode", "SubMode: " + subModeName);
    } else if (currentMode == MODE_DIAGNOSE_ENCODER) {
      // 在编码器诊断模式下，切换子显示模式
      encoderSubMode = (encoderSubMode + 1) % 2;  // 2个子模式循环切换
      
      String subModeName = encoderSubMode == 0 ? "位置显示" : "相位变化";
      Serial.println("[DIAGNOSTIC] 切换到子模式: " + subModeName);
      userInterface->displayDiagnosticInfo("Encoder Diag", "SubMode: " + subModeName);
    } else if (currentMode == MODE_DIAGNOSE_OUTLET) {
      // 在出口诊断模式下，切换子显示模式
      outletSubMode = (outletSubMode + 1) % 2;  // 2个子模式循环切换
      
      String subModeName = outletSubMode == 0 ? "轮巡降落（常态打开）" : "轮巡上升（常态闭合）";
      Serial.println("[DIAGNOSTIC] 切换到子模式: " + subModeName);
      userInterface->displayDiagnosticInfo("Outlet Diag", "SubMode: " + subModeName);
    } else {
      // 其他模式下，从按钮功能处理（当前未使用，可根据需要扩展）
      Serial.println("[DIAGNOSTIC] Slave button pressed");
    }
    // 注意：isSlaveButtonPressed()方法会自动清除标志
  }
}

// 处理模式切换
void handleModeChange() {
  if (modeChangePending) {
    // 如果当前是MODE_DIAGNOSE_SCANNER模式，切换前重置诊断模式显示标志
    if (currentMode == MODE_DIAGNOSE_SCANNER) {
      userInterface->resetDiagnosticMode();
    }
    
    // 如果当前是MODE_NORMAL模式，切换前重置子模式初始化标志
    if (currentMode == MODE_NORMAL) {
      // 通过设置一个标志来重置子模式初始化状态
      // 这里不需要做任何操作，因为subModeInitialized是静态变量
      // 我们会在MODE_NORMAL的case中处理初始化
    }
    
    // 应用模式切换
    currentMode = pendingMode;
    modeChangePending = false;
    
    // 如果切换到MODE_VERSION_INFO模式，重置版本信息已显示标志
    if (currentMode == MODE_VERSION_INFO) {
      versionInfoAlreadyDisplayed = false;
    }
    
    // 如果切换到MODE_NORMAL模式，重置子模式为0
    if (currentMode == MODE_NORMAL) {
      normalSubMode = 0;
    }
    
    // 移除模式LED控制，LED现在只用于显示出口状态
    
    // 打印模式切换完成信息
    Serial.print("[DIAGNOSTIC] Mode switched to: ");
    Serial.println(getModeName(currentMode));
    
    // 显示模式变化信息到OLED
    userInterface->displayModeChange(currentMode);
  }
}

// 处理编码器诊断模式
void processDiagnoseEncoderMode() {
  // 诊断编码器模式 - 根据子模式显示不同信息
  static bool subModeInitialized = false;
  
  if (!subModeInitialized) {
    subModeInitialized = true;
    Serial.println("[DIAGNOSTIC] Encoder Diagnostic Mode Activated");
    Serial.println("[DIAGNOSTIC] 子模式: " + String(encoderSubMode == 0 ? "位置显示" : "相位变化"));
    Serial.println("[DIAGNOSTIC] 使用从按钮切换子模式");
  }
  
  // 根据子模式执行相应功能
  // 子模式0: 显示编码器位置（只在位置变化时更新）
  // 子模式1: 显示相位变化信息（只在相位变化时更新）
  // 注意：实际显示由OLED的update方法处理
}

// 处理扫描仪诊断模式
void processDiagnoseScannerMode() {
  // 诊断扫描仪模式 - 子测试模式
  static int scannerSubMode = 0;  // 0: IO状态检查, 1: 直径测试, 2: 循环切换
  static bool subModeInitialized = false;
  
  if (!subModeInitialized) {
    subModeInitialized = true;
    Serial.println("[DIAGNOSTIC] Scanner Diagnostic Mode Activated");
    Serial.println("[DIAGNOSTIC] 子模式: IO状态检查");
    Serial.println("[DIAGNOSTIC] 使用从按钮切换子模式");
  }
  
  // 根据子模式执行相应功能
  if (scannerSubMode == 0) {
    sorter.displayIOStatus();
  } else if (scannerSubMode == 1) {
    sorter.displayRawDiameters();
  }
}

// 处理出口诊断模式
void processDiagnoseOutletMode(unsigned long currentTime) {
  // 诊断出口模式 - 根据子模式测试出口
  static unsigned long modeStartTime = 0;
  static unsigned long lastOutletTime = 0;
  static bool outletState = false;
  static uint8_t currentOutlet = 0;
  static uint8_t MAX_OUTLETS = 0;
  static bool displayInitialized = false;
  
  // 模式开始时初始化
  if (modeStartTime == 0) {
    modeStartTime = currentTime;
    lastOutletTime = currentTime;
    outletState = false;
    currentOutlet = 0;
    displayInitialized = false;
    // 获取出口数量
    MAX_OUTLETS = sorter.getOutletCount();
    Serial.println("[DIAGNOSTIC] 诊断出口模式已启动 - 子模式: " + String(outletSubMode == 0 ? "轮巡降落（常态打开）" : "轮巡上升（常态闭合）"));
    Serial.println("[DIAGNOSTIC] 使用从按钮切换子模式");
  }
  
  // 初始化显示
  if (!displayInitialized) {
    displayInitialized = true;
    userInterface->displayOutletTestGraphic(MAX_OUTLETS, 255, outletSubMode);  // 初始状态，没有打开的出口
  }
  
  // 根据子模式执行相对独立的逻辑
  switch (outletSubMode) {
    case 0: {
      // 子模式0：轮巡降落（常态打开，偶尔闭合）
      // 打开保持时间：4.5秒，关闭保持时间：1.5秒
      unsigned long interval = outletState ? 4500 : 1500;
      
      if (currentTime - lastOutletTime >= interval) {
        lastOutletTime = currentTime;
        outletState = !outletState;
        
        // 当状态从关闭切换到打开时，移动到下一个出口
        if (outletState) {
          currentOutlet = (currentOutlet + 1) % MAX_OUTLETS;
          Serial.print("[DIAGNOSTIC] 现在测试出口常态关闭: ");
          Serial.println(currentOutlet);
        }
        
        // 使用公共方法控制当前出口
        sorter.setOutletState(currentOutlet, outletState);
        
        // 更新OLED显示
        if (outletState) {
          userInterface->displayOutletTestGraphic(MAX_OUTLETS, currentOutlet, outletSubMode);
        } else {
          userInterface->displayOutletTestGraphic(MAX_OUTLETS, 255, outletSubMode);  // 255表示没有打开的出口
        }
      }
      break;
    }
    
    case 1: {
      // 子模式1：轮巡上升（常态闭合，偶尔打开）
      // 关闭保持时间：4.5秒，打开保持时间：1.5秒
      unsigned long interval = outletState ? 1500 : 4500;
      
      if (currentTime - lastOutletTime >= interval) {
        lastOutletTime = currentTime;
        outletState = !outletState;
        
        // 当状态从关闭切换到打开时，移动到下一个出口
        if (outletState) {
          currentOutlet = (currentOutlet + 1) % MAX_OUTLETS;
          Serial.print("[DIAGNOSTIC] 现在测试出口常态打开: ");
          Serial.println(currentOutlet);
        }
        
        // 使用公共方法控制当前出口
        sorter.setOutletState(currentOutlet, outletState);
        
        // 更新OLED显示
        if (outletState) {
          userInterface->displayOutletTestGraphic(MAX_OUTLETS, currentOutlet, outletSubMode);
        } else {
          userInterface->displayOutletTestGraphic(MAX_OUTLETS, 255, outletSubMode);  // 255表示没有打开的出口
        }
      }
      break;
    }
  }
}

// 处理上料器测试模式
void processTestReloaderMode(unsigned long currentTime) {
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
}

// 处理版本信息模式
void processVersionInfoMode() {
  // 版本信息模式 - 显示机器型号、版本号、作者版权、年份和电话
  // 这个变量就像"煎饼已经煎好了"的标记，防止重复煎饼（重复显示）
  // 就像吃药一样，每天吃一次就够了，不需要每分钟都吃一次药
  // 版本信息只需要显示一次，如果每次循环都重新显示，OLED会不断闪烁
  if (!versionInfoAlreadyDisplayed) {
    versionInfoAlreadyDisplayed = true;
    Serial.println("[VERSION] Version Info Mode Activated");
    
    // 显示版本信息到OLED
    String versionInfo = "\n\nAsparagus sorter\n\n";
    versionInfo += "Tel: 133-0640-0990";
    userInterface->displayDiagnosticInfo(systemName, versionInfo);
  }
}

// 处理正常模式
void processNormalMode() {
  // 正常模式 - 根据子模式显示不同信息
  static bool subModeInitialized = false;
  
  if (!subModeInitialized) {
    subModeInitialized = true;
    Serial.println("[NORMAL] Normal Mode Activated");
    Serial.println("[NORMAL] 子模式: " + String(normalSubMode == 0 ? "统计信息" : "最新直径"));
    Serial.println("[NORMAL] 使用从按钮切换子模式");
  }
  
  // 根据子模式执行相应功能
  // 子模式0: 显示统计信息（速度、托架数量、识别数量）
  // 子模式1: 显示最新直径（名义直径）
  // 注意：实际显示由OLED的update方法处理
}

void loop() {
  // 处理按钮输入
  handleMasterButton();
  handleSlaveButton();
  
  // 处理模式切换
  handleModeChange();
  
  // 处理当前工作模式
  unsigned long currentTime = millis();
  
  switch (currentMode) {
    case MODE_DIAGNOSE_ENCODER:
      processDiagnoseEncoderMode();
      break;
      
    case MODE_DIAGNOSE_SCANNER:
      processDiagnoseScannerMode();
      break;
      
    case MODE_DIAGNOSE_OUTLET:
      processDiagnoseOutletMode(currentTime);
      break;
      
    case MODE_TEST_RELOADER:
      processTestReloaderMode(currentTime);
      break;
      
    case MODE_VERSION_INFO:
      processVersionInfoMode();
      break;
      
    case MODE_NORMAL:
      processNormalMode();
      break;
      
    default:
      // 正常模式下，系统通过编码器相位变化触发Sorter的onPhaseChange方法
      break;
  }
  
  // 处理编码器触发的任务（在所有模式下都执行）
  sorter.spinOnce();
  
  // 更新OLED显示内容
  DisplayData displayData = sorter.getDisplayData(currentMode, normalSubMode, encoderSubMode, outletSubMode);
  userInterface->updateDisplay(displayData);
}

// 函数实现

String getModeName(SystemMode mode) {
  // 获取指定模式名称
  switch (mode) {
    case MODE_NORMAL:
      return "正常模式";
    case MODE_DIAGNOSE_ENCODER:
      return "编码器诊断模式";
    case MODE_DIAGNOSE_SCANNER:
      return "扫描仪诊断模式";
    case MODE_DIAGNOSE_OUTLET:
      return "出口诊断模式";
    case MODE_TEST_RELOADER:
      return "上料器测试模式（Feeder Test Mode）";
    case MODE_VERSION_INFO:
      return "版本信息模式";
    default:
      return "未知模式";
  }
}