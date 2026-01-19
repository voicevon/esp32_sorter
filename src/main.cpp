#include <Arduino.h>

// #include "outlet.h"
#include "user_interface/user_interface.h"
#include "modular/encoder.h"
#include "modular/sorter.h"
#include "scanner_diagnostic_handler.h"
#include "outlet_diagnostic_handler.h"
#include "reloader_test_handler.h"

// 引入系统工作模式定义
#include "main.h"

// 全局变量定义

// =========================
// 系统配置与版本信息
// =========================
// 版本信息
String firmwareVersion = "ver: 2601";

// =========================
// 模式相关变量
// =========================
// 当前工作模式
SystemMode currentMode = MODE_NORMAL;
// 待切换模式
SystemMode pendingMode = MODE_NORMAL;
// 模式切换标志
bool modeChangePending = false;

// =========================
// 子模式变量
// =========================
// 正常模式子模式（0: 统计信息, 1: 最新直径）
int normalModeSubmode = 0;
// 编码器诊断模式子模式（0: 位置显示, 1: 相位变化）
int encoderDiagnosticSubmode = 0;
// 出口测试模式子模式（0: 轮巡降落, 1: 轮巡上升）
int outletTestSubmode = 0;

// =========================
// 状态标志
// =========================
// 版本信息已显示标志
bool hasVersionInfoDisplayed = false;

// =========================
// 单例实例
// =========================
// 人机交互模块实例
UserInterface* userInterface = UserInterface::getInstance();
// 编码器实例
Encoder* encoder = Encoder::getInstance();
// 直径扫描仪实例
DiameterScanner* diameterScanner = DiameterScanner::getInstance();

// =========================
// 系统组件实例
// =========================
// 芦笋托盘系统实例
TrayManager traySystem;
// 分拣控制器实例
Sorter sorter;
// 扫描仪诊断处理类实例
ScannerDiagnosticHandler scannerDiagnosticHandler;
// 出口诊断处理类实例
OutletDiagnosticHandler outletDiagnosticHandler;
// 上料器测试处理类实例
ReloaderTestHandler reloaderTestHandler;

// 函数声明
String getSystemModeName(SystemMode mode);

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("ESP32 Sorter system starting...");
  
  // 初始化用户界面（包含OLED和SimpleHMI）
  userInterface->initialize();
  
  // 显式启用所有输出渠道（串口和OLED）
  userInterface->enableOutputChannel(OUTPUT_ALL);
  
  // 串口初始化完成后无需等待连接建立
  
  // 初始化编码器
  encoder->initialize();
  
  // 出口位置初始化已移至Sorter类的initialize方法中
  
  // 初始化Sorter
  sorter.initialize();
  
  // 初始化出口诊断处理类
  outletDiagnosticHandler.initialize();
  
  // 初始化上料器测试处理类
  reloaderTestHandler.initialize();
  
  Serial.println("System ready");
  Serial.println("Current Mode: " + getSystemModeName(currentMode));
  Serial.println("Use mode button to switch between modes");
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
      Serial.println(getSystemModeName(currentMode));
      currentMode = tempMode; // 恢复原模式
    // 注意：isMasterButtonPressed()方法会自动清除标志
  }
}

// 处理从按钮（子模式切换）
void handleSlaveButton() {
  if (userInterface->isSlaveButtonPressed()) {
    // 从按钮功能处理
    if (currentMode == MODE_NORMAL) {
      // 在正常模式下，切换子显示模式
      normalModeSubmode = (normalModeSubmode + 1) % 2;  // 2个子模式循环切换
      
      String subModeName = normalModeSubmode == 0 ? "Stats" : "Latest Diameter";
      Serial.println("[NORMAL] Switch to Submode: " + subModeName);
      userInterface->displayDiagnosticInfo("Normal Mode", "SubMode: " + subModeName);
    } else if (currentMode == MODE_DIAGNOSE_ENCODER) {
      // 在编码器诊断模式下，切换子显示模式
      encoderDiagnosticSubmode = (encoderDiagnosticSubmode + 1) % 2;  // 2个子模式循环切换
      
      String subModeName = encoderDiagnosticSubmode == 0 ? "Position" : "Phase Change";
      Serial.println("[DIAGNOSTIC] Switch to Submode: " + subModeName);
      userInterface->displayDiagnosticInfo("Encoder Diag", "SubMode: " + subModeName);
    } else if (currentMode == MODE_DIAGNOSE_OUTLET) {
      // 在出口诊断模式下，切换子显示模式
      outletTestSubmode = (outletTestSubmode + 1) % 2;  // 2个子模式循环切换
      
      String subModeName = outletTestSubmode == 0 ? "Cycle Drop (Normally Open)" : "Cycle Raise (Normally Closed)";
      Serial.println("[DIAGNOSTIC] Switch to Submode: " + subModeName);
      userInterface->displayDiagnosticInfo("Outlet Diag", "SubMode: " + subModeName);
    } else if (currentMode == MODE_DIAGNOSE_SCANNER) {
      // 在扫描仪诊断模式下，切换子显示模式
      scannerDiagnosticHandler.switchToNextSubMode();
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
      // 确保在退出扫描仪诊断模式时禁用编码器值记录模式

      Serial.println("[DIAGNOSTIC] 已禁用扫描边缘校准模式");
    }
    
    // 应用模式切换
    currentMode = pendingMode;
    modeChangePending = false;
    
    // 如果切换到MODE_VERSION_INFO模式，重置版本信息已显示标志
        if (currentMode == MODE_VERSION_INFO) {
          hasVersionInfoDisplayed = false;
        }
    
    // 如果切换到MODE_NORMAL模式，重置子模式为0
    if (currentMode == MODE_NORMAL) {
      normalModeSubmode = 0;
    }
    
    // 移除模式LED控制，LED现在只用于显示出口状态
    
    // 打印模式切换完成信息
      Serial.print("[DIAGNOSTIC] Mode switched to: ");
      Serial.println(getSystemModeName(currentMode));
    
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
    Serial.println("[DIAGNOSTIC] Submode: " + String(encoderDiagnosticSubmode == 0 ? "Position" : "Phase Change"));
    Serial.println("[DIAGNOSTIC] Use slave button to switch submode");
  }
  
  // 获取编码器实例
  Encoder* encoder = Encoder::getInstance();
  
  // 获取编码器位置信息
  int encoderPosition = encoder->getCurrentPosition();
  bool positionChanged = encoder->hasPositionChanged();
  
  // 使用新的显示方法
  userInterface->displayPositionInfo("Encoder", encoderPosition, true);
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
    MAX_OUTLETS = outletDiagnosticHandler.getOutletCount();
    Serial.println("[DIAGNOSTIC] Outlet Diagnostic Mode Activated - Submode: " + String(outletTestSubmode == 0 ? "Cycle Drop (Normally Open)" : "Cycle Raise (Normally Closed)"));
    Serial.println("[DIAGNOSTIC] Use slave button to switch submode");
  }
  
  // 初始化显示
  if (!displayInitialized) {
    displayInitialized = true;
    userInterface->displayOutletTestGraphic(MAX_OUTLETS, 255, outletTestSubmode);  // 初始状态，没有打开的出口
  }
  
  // 根据子模式执行相对独立的逻辑
  switch (outletTestSubmode) {
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
          Serial.print("[DIAGNOSTIC] Now testing outlet normally closed: ");
          Serial.println(currentOutlet);
        }
        
        // 使用公共方法控制当前出口
        outletDiagnosticHandler.setOutletState(currentOutlet, outletState);
        
        // 更新OLED显示
        if (outletState) {
          userInterface->displayOutletTestGraphic(MAX_OUTLETS, currentOutlet, outletTestSubmode);
        } else {
          userInterface->displayOutletTestGraphic(MAX_OUTLETS, 255, outletTestSubmode);  // 255表示没有打开的出口
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
          Serial.print("[DIAGNOSTIC] Now testing outlet normally open: ");
          Serial.println(currentOutlet);
        }
        
        // 使用公共方法控制当前出口
        outletDiagnosticHandler.setOutletState(currentOutlet, outletState);
        
        // 更新OLED显示
        if (outletState) {
          userInterface->displayOutletTestGraphic(MAX_OUTLETS, currentOutlet, outletTestSubmode);
        } else {
          userInterface->displayOutletTestGraphic(MAX_OUTLETS, 255, outletTestSubmode);  // 255表示没有打开的出口
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
        reloaderTestHandler.openReloader();  // 使用公共方法开启上料器
      } else {
        reloaderTestHandler.closeReloader(); // 使用公共方法关闭上料器
      }
    }
  } else {
    // 15秒后，关闭并保持5秒
    if (reloaderState) {
      reloaderTestHandler.closeReloader();   // 使用公共方法确保关闭
      reloaderState = false;
      Serial.println("[DIAGNOSTIC] Feeder closed and maintained after 15 seconds");
    }
  }
}

// 处理版本信息模式
void processVersionInfoMode() {
  // 版本信息模式 - 显示机器型号、版本号、作者版权、年份和电话
  // This variable acts like a flag for "pancake is ready", preventing duplicate pancakes (duplicate displays)
  // Like taking medicine, once a day is enough, no need to take it every minute
  // Version info only needs to be displayed once; if redisplayed every loop, OLED will keep flickering
  if (!hasVersionInfoDisplayed) {
    hasVersionInfoDisplayed = true;
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
    Serial.println("[NORMAL] Submode: " + String(normalModeSubmode == 0 ? "Stats" : "Latest Diameter"));
    Serial.println("[NORMAL] Use slave button to switch submode");
  }
  
  // 根据子模式调用对应的专用显示方法
  if (normalModeSubmode == 0) {
    // 子模式0：统计信息
    float speedPerSecond = sorter.getConveyorSpeedPerSecond();
    int speedPerMinute = speedPerSecond * 60.0f;
    int speedPerHour = speedPerSecond * 3600.0f;
    
    // 从各个组件获取数据
    DiameterScanner* scanner = DiameterScanner::getInstance();
    int identifiedCount = scanner->getTotalObjectCount();
    
    // 计算已输送的托架数量（每200个编码器脉冲对应一个托架移动）
    const int pulsesPerTray = 200;
    long encoderPosition = encoder->getRawCount();
    int transportedTrayCount = encoderPosition / pulsesPerTray;
    
    // 调用专用显示方法
    userInterface->displayDashboard(speedPerSecond, speedPerMinute, speedPerHour, identifiedCount, transportedTrayCount);
  } else {
    // 子模式1：最新直径
    int latestDiameter = traySystem.getTrayDiameter(0);
    
    // 调用专用显示方法
    userInterface->displayNormalModeDiameter(latestDiameter);
  }
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
    case MODE_DIAGNOSE_SCANNER:
      // 处理从按钮输入，切换子模式
      if (userInterface->isSlaveButtonPressed()) {
        scannerDiagnosticHandler.switchToNextSubMode();
      }
      
      // 执行诊断模式的主要逻辑
      scannerDiagnosticHandler.update();
      break;
      
    case MODE_DIAGNOSE_OUTLET:
      processDiagnoseOutletMode(currentTime);
      outletDiagnosticHandler.processTasks();  // 出口诊断模式需要处理出口相关任务
      break;
      
    case MODE_TEST_RELOADER:
      processTestReloaderMode(currentTime);
      reloaderTestHandler.processTasks();  // 上料器测试模式需要处理上料器相关任务
      break;
      
    case MODE_VERSION_INFO:
      processVersionInfoMode();
      // 版本信息模式不需要处理任何任务
      break;
      
    case MODE_DIAGNOSE_ENCODER:
      processDiagnoseEncoderMode();
      // 编码器诊断模式不需要处理任何任务，只需要监控编码器状态
      break;
      
    case MODE_NORMAL:
      processNormalMode();
      // 正常模式需要处理所有任务
      sorter.processScannerTasks();
      sorter.processOutletTasks();
      sorter.processReloaderTasks();
      break;
      
    default:
      // 未知模式，输出警告信息
      Serial.print("Warning: Encountered unhandled system mode: ");
      Serial.println(currentMode);
      break;
  }
  

  

}

// 函数实现

String getSystemModeName(SystemMode mode) {
  // 获取指定模式名称
  switch (mode) {
    case MODE_NORMAL:
      return "Normal Mode";
    case MODE_DIAGNOSE_ENCODER:
      return "Encoder Diag";
    case MODE_DIAGNOSE_SCANNER:
      return "Scanner Diag";
    case MODE_DIAGNOSE_OUTLET:
      return "Outlet Diag";
    case MODE_TEST_RELOADER:
      return "Feeder Test";
    case MODE_VERSION_INFO:
      return "Version Info";
    default:
      return "Unknown Mode";
  }
}