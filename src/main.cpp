#include <Arduino.h>
#include <EEPROM.h>

// #include "outlet.h"
#include "user_interface/user_interface.h"
#include "modular/encoder.h"
#include "modular/sorter.h"
#include "scanner_diagnostic_handler.h"
#include "outlet_diagnostic_handler.h"
#include "encoder_diagnostic_handler.h"
#include "config_handler.h"

// 引入系统工作模式定义
#include "main.h"
#include "user_interface/oled.h"
#include "user_interface/terminal.h"

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
// 编码器诊断处理类实例
EncoderDiagnosticHandler encoderDiagnosticHandler;
// 配置处理类实例
DiameterConfigHandler diameterConfigHandler(userInterface, &sorter);
OutletPosConfigHandler outletPosConfigHandler(userInterface, &sorter);

// 函数声明
String getSystemModeName(SystemMode mode);

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("Feng's Sorter system starting...");
  
  // 初始化用户界面（不包含显示设备）
  userInterface->initialize();
  
  // 创建并初始化显示设备
  OLED* oled = OLED::getInstance();
  Terminal* terminal = Terminal::getInstance();
  
  oled->initialize();
  terminal->initialize();
  
  // 将显示设备注入到UserInterface中
  UserInterface::addExternalDisplayDevice(oled);
  UserInterface::addExternalDisplayDevice(terminal);
  
  // 显式启用所有输出渠道（串口和OLED）
  userInterface->enableOutputChannel(OUTPUT_ALL);
  
  // 设置每个出口对象的指针到出口诊断处理类
  for (uint8_t i = 0; i < OUTLET_COUNT; i++) {
    outletDiagnosticHandler.setOutlet(i, sorter.getOutlet(i));
  }
  
  // 串口初始化完成后无需等待连接建立
  
  // 初始化编码器
  encoder->initialize();
  
  // 出口位置初始化已移至Sorter类的initialize方法中
  
  // 初始化Sorter
  sorter.initialize();
  
  // 初始化出口诊断处理类，并传入UserInterface指针
  outletDiagnosticHandler.initialize(userInterface);
  
  // 初始化编码器诊断处理类，并传入UserInterface指针
  encoderDiagnosticHandler.initialize(userInterface);
  
  Serial.println("System ready");
  Serial.println("Current Mode: " + getSystemModeName(currentMode));
  Serial.println("Use mode button to switch between modes");
  
  // 测试代码已注释：自动触发模式切换的测试功能已禁用
  // delay(2000);
  // pendingMode = static_cast<SystemMode>((currentMode + 1) % 6);
  // modeChangePending = true;
}

// 处理主按钮（模式切换）
void handleMasterButton() {
  // 长按触发模式切换
  if (userInterface->isMasterButtonLongPressed()) {
    // 切换到下一个工作模式
    pendingMode = static_cast<SystemMode>((currentMode + 1) % 8); // 8种模式循环切换
    modeChangePending = true;
    
    // 打印模式切换请求信息
    Serial.print("[DIAGNOSTIC] Mode switch requested to: ");
    // 临时保存当前模式，显示待切换的模式名称
      SystemMode tempMode = currentMode;
      currentMode = pendingMode;
      Serial.println(getSystemModeName(currentMode));
      currentMode = tempMode; // 恢复原模式
  } else if (userInterface->isMasterButtonPressed()) {
    // 短按处理（可根据需要扩展）
    Serial.println("[DIAGNOSTIC] Master button short pressed");
  }
}

// 处理从按钮（子模式切换）
void handleSlaveButton() {
  // 长按触发子模式切换
  if (userInterface->isSlaveButtonLongPressed()) {
    // 从按钮功能处理
    if (currentMode == MODE_NORMAL) {
      // 在正常模式下，切换子显示模式
      normalModeSubmode = (normalModeSubmode + 1) % 2;  // 2个子模式循环切换
      
      String subModeName = normalModeSubmode == 0 ? "Stats" : "Latest Diameter";
      Serial.println("[NORMAL] Switch to Submode: " + subModeName);
      userInterface->displayDiagnosticInfo("Normal Mode", "SubMode: " + subModeName);
    } else if (currentMode == MODE_DIAGNOSE_ENCODER) {
      // 在编码器诊断模式下，切换子显示模式
      encoderDiagnosticHandler.switchToNextSubMode();
    } else if (currentMode == MODE_DIAGNOSE_OUTLET) {
      // 在出口诊断模式下，切换子显示模式
      outletDiagnosticHandler.switchToNextSubMode();
    } else if (currentMode == MODE_DIAGNOSE_SCANNER) {
      // 在扫描仪诊断模式下，切换子显示模式
      scannerDiagnosticHandler.switchToNextSubMode();
    } else {
      // 其他模式下，从按钮功能处理（当前未使用，可根据需要扩展）
      Serial.println("[DIAGNOSTIC] Slave button long pressed");
    }
  } else if (userInterface->isSlaveButtonPressed()) {
    // 短按处理（可根据需要扩展）
    Serial.println("[DIAGNOSTIC] Slave button short pressed");
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
    
    // 如果当前是配置模式，退出时写入EEPROM
    if (currentMode == MODE_CONFIG_DIAMETER || currentMode == MODE_CONFIG_OUTLET_POS) {
      Serial.println("[CONFIG] Saving configuration to EEPROM...");
      
      // 初始化EEPROM
      EEPROM.begin(512);
      
      // 定义EEPROM中存储直径范围的起始地址
      const int EEPROM_DIAMETER_RANGES_ADDR = 0;
      // 定义EEPROM中存储舵机位置的起始地址
      const int EEPROM_SERVO_POSITIONS_ADDR = 0x12;
      const int EEPROM_SERVO_MAGIC_ADDR = 0x32;
      
      // 写入直径范围数据
      EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR, 0xAA); // 写入魔术字节
      for (uint8_t i = 0; i < OUTLET_COUNT; i++) {
        EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2, sorter.getOutletMinDiameter(i));
        EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2 + 1, sorter.getOutletMaxDiameter(i));
      }
      
      // 写入舵机位置数据
      EEPROM.write(EEPROM_SERVO_MAGIC_ADDR, 0xBB); // 写入魔术字节
      for (uint8_t i = 0; i < OUTLET_COUNT; i++) {
        EEPROM.write(EEPROM_SERVO_POSITIONS_ADDR + i, sorter.getOutletClosedPosition(i));
        EEPROM.write(EEPROM_SERVO_POSITIONS_ADDR + OUTLET_COUNT + i, sorter.getOutletOpenPosition(i));
      }
      
      // 提交写入
      EEPROM.commit();
      Serial.println("[CONFIG] Configuration saved to EEPROM");
    }
    
    // 保存旧模式
    SystemMode oldMode = currentMode;
    
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
    
    // 如果从配置模式切换出，重置配置处理类状态
    if ((oldMode == MODE_CONFIG_DIAMETER || oldMode == MODE_CONFIG_OUTLET_POS) && 
        (currentMode != MODE_CONFIG_DIAMETER && currentMode != MODE_CONFIG_OUTLET_POS)) {
      // 重置配置处理类状态
      diameterConfigHandler.reset();
      outletPosConfigHandler.reset();
    }
    
    // 移除模式LED控制，LED现在只用于显示出口状态
    
    // 打印模式切换完成信息
      Serial.print("[DIAGNOSTIC] Mode switched to: ");
      Serial.println(getSystemModeName(currentMode));
    
    // 显示模式变化信息到OLED
    userInterface->displayModeChange(currentMode);
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
    case MODE_NORMAL:
      processNormalMode();
      // 正常模式需要处理所有任务
      sorter.processScannerTasks();
      sorter.processOutletTasks();
      break;
      
    case MODE_DIAGNOSE_ENCODER:
      // 处理从按钮输入，切换子模式
      if (userInterface->isSlaveButtonPressed()) {
        encoderDiagnosticHandler.switchToNextSubMode();
      }
      
      // 执行诊断模式的主要逻辑
      encoderDiagnosticHandler.update();
      break;
      
    case MODE_DIAGNOSE_SCANNER:
      // 处理从按钮输入，切换子模式
      if (userInterface->isSlaveButtonPressed()) {
        scannerDiagnosticHandler.switchToNextSubMode();
      }
      
      // 执行诊断模式的主要逻辑
      scannerDiagnosticHandler.update();
      break;
      
    case MODE_DIAGNOSE_OUTLET:
      // 处理从按钮输入，切换子模式
      if (userInterface->isSlaveButtonPressed()) {
        outletDiagnosticHandler.switchToNextSubMode();
      }
      
      // 执行诊断模式的主要逻辑
      outletDiagnosticHandler.update(currentTime);
      break;
      

      
    case MODE_CONFIG_DIAMETER:
      diameterConfigHandler.update();
      break;
      
    case MODE_CONFIG_OUTLET_POS:
      outletPosConfigHandler.update();
      break;
      
    case MODE_VERSION_INFO:
      processVersionInfoMode();
      // 版本信息模式不需要处理任何任务
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
    case MODE_VERSION_INFO:
      return "Version Info";
    case MODE_CONFIG_DIAMETER:
      return "Config Diameter";
    case MODE_CONFIG_OUTLET_POS:
      return "Config Outlet Pos";
    default:
      return "Unknown Mode";
  }
}

