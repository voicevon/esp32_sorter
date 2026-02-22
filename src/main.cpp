#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#define MAIN_H_SOURCE
#include "main.h"
#include "config.h"
#include "user_interface/user_interface.h"
#include "user_interface/oled.h"
#include "user_interface/terminal.h"
#include "user_interface/menu_system.h"
#include "modular/encoder.h"
#include "modular/sorter.h"
#include "scanner_diagnostic_handler.h"
#include "outlet_diagnostic_handler.h"
#include "encoder_diagnostic_handler.h"
#include "config_handler.h"

// 全局变量定义

// =========================
// 系统配置与版本信息
// =========================
// 系统名称
String systemName = "Feng's AS-L9";
// 版本信息
String firmwareVersion = "ver: 2601";
// 启动计数
unsigned long systemBootCount = 0;

// =========================
// 模式相关变量
// =========================
// 当前工作模式
SystemMode currentMode = MODE_NORMAL;
// 待切换模式
SystemMode pendingMode = MODE_NORMAL;
// 模式切换标志
bool modeChangePending = false;

// 菜单系统与节点
bool menuModeActive = true;
MenuSystem menuSystem(5);
MenuNode rootMenu("Main Menu");
MenuNode diagMenu("Diagnostics", &rootMenu);
MenuNode outletDiagMenu("Outlet Diag", &diagMenu);
MenuNode configMenu("Configurations", &rootMenu);

// 函数：切换模式辅助
void switchToMode(SystemMode mode) {
    pendingMode = mode;
    modeChangePending = true;
    menuModeActive = false;
}

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
// 芦笋托盘系统实例（单例模式）
TraySystem* allTrays = TraySystem::getInstance();
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

// 函数声明
String getSystemModeName(SystemMode mode);

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("Feng's Sorter system starting...");
  
  // 初始化用户界面
  userInterface->initialize();
  
  // 创建并初始化显示设备
  OLED* oled = OLED::getInstance();
  Terminal* terminal = Terminal::getInstance();
  
  oled->initialize();
  terminal->initialize();
  
  // 将显示设备注册到 UserInterface 管理器中
  userInterface->addDisplayDevice(oled);
  userInterface->addDisplayDevice(terminal);
  
  // 显式启用所有输出渠道（串口和OLED）
  userInterface->enableOutputChannel(OUTPUT_ALL);
  
  // 显式启用所有输出渠道（串口和OLED）
  userInterface->enableOutputChannel(OUTPUT_ALL);
  
  // 初始化电源监控引脚
  pinMode(PIN_POWER_MONITOR, INPUT);
  
  // 设置每个出口对象的指针到出口诊断处理类
  for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
    outletDiagnosticHandler.setOutlet(i, sorter.getOutlet(i));
  }
  
  // 串口初始化完成后无需等待连接建立
  
  // 初始化EEPROM并处理启动计数
  EEPROM.begin(512); // Ensure EEPROM is initialized
  EEPROM.get(EEPROM_ADDR_BOOT_COUNT, systemBootCount);
  
  // 如果读取值为0xFFFFFFFF（首次使用或擦除后），重置为0
  if (systemBootCount == 0xFFFFFFFF) {
      systemBootCount = 0;
  }
  
  // 增加计数并保存
  systemBootCount++;
  EEPROM.put(EEPROM_ADDR_BOOT_COUNT, systemBootCount);
  EEPROM.commit();
  Serial.print("Boot Count: ");
  Serial.println(systemBootCount);
  
  // 初始化编码器
  encoder->initialize();
  
  // 出口位置初始化已移至Sorter类的initialize方法中
  
  // 初始化Sorter
  sorter.initialize();
  
  // 尝试从EEPROM恢复托盘数据（用于断电恢复）
  allTrays->loadFromEEPROM(EEPROM_ADDR_TRAY_DATA);
  
  // 初始化出口诊断处理类，并传入UserInterface指针
  outletDiagnosticHandler.initialize(userInterface);
  
  // 将 Sorter 管理的出口对象注入到诊断处理器中
  for (int i = 0; i < NUM_OUTLETS; i++) {
      outletDiagnosticHandler.setOutlet(i, sorter.getOutlet(i));
  }
  
  // 初始化编码器诊断处理类，并传入UserInterface指针
  encoderDiagnosticHandler.initialize(userInterface);
  
  // =========================
  // 建立菜单树
  // =========================
  // 设置菜单旋转灵敏度：由于采用了 4 倍频采样，此处设置为 4 代表每一格物理刻度移动一行
  menuSystem.setSensitivity(4); 
  
  rootMenu.addItem(MenuItem("Run Sorter", MENU_TYPE_ACTION, nullptr, [](){
      switchToMode(MODE_NORMAL);
  }));
  rootMenu.addItem(MenuItem("Diagnostics >", MENU_TYPE_SUBMENU, &diagMenu));
  rootMenu.addItem(MenuItem("Configuration >", MENU_TYPE_SUBMENU, &configMenu));
  rootMenu.addItem(MenuItem("Version Info", MENU_TYPE_ACTION, nullptr, [](){
      switchToMode(MODE_VERSION_INFO);
  }));

  diagMenu.addItem(MenuItem("Encoder Test", MENU_TYPE_ACTION, nullptr, [](){
      switchToMode(MODE_DIAGNOSE_ENCODER);
  }));
  diagMenu.addItem(MenuItem("Scanner Test", MENU_TYPE_ACTION, nullptr, [](){
      switchToMode(MODE_DIAGNOSE_SCANNER);
  }));
  diagMenu.addItem(MenuItem("Outlet Diag >", MENU_TYPE_SUBMENU, &outletDiagMenu));
  diagMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

  outletDiagMenu.addItem(MenuItem("Cycle Drop (NC)", MENU_TYPE_ACTION, nullptr, [](){
      outletDiagnosticHandler.setSubMode(0);
      switchToMode(MODE_DIAGNOSE_OUTLET);
  }));
  outletDiagMenu.addItem(MenuItem("Cycle Raise (NO)", MENU_TYPE_ACTION, nullptr, [](){
      outletDiagnosticHandler.setSubMode(1);
      switchToMode(MODE_DIAGNOSE_OUTLET);
  }));
  outletDiagMenu.addItem(MenuItem("Lifetime Test", MENU_TYPE_ACTION, nullptr, [](){
      outletDiagnosticHandler.setSubMode(2);
      switchToMode(MODE_DIAGNOSE_OUTLET);
  }));
  outletDiagMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

  configMenu.addItem(MenuItem("Diameter Ranges", MENU_TYPE_ACTION, nullptr, [](){
      switchToMode(MODE_CONFIG_DIAMETER);
  }));
  configMenu.addItem(MenuItem("< Back", MENU_TYPE_BACK));

  menuSystem.setRootMenu(&rootMenu);

  Serial.println("System ready");
  Serial.println("Current Mode: " + getSystemModeName(currentMode));
  Serial.println("Use encoder knob to navigate and select");
  
  // 测试代码已注释：自动触发模式切换的测试功能已禁用
  // delay(2000);
  // pendingMode = static_cast<SystemMode>((currentMode + 1) % 6);
  // modeChangePending = true;
}

// 处理返回主菜单（长按退出当前模式）
void handleReturnToMenu() {
    // 设置返回主菜单标志
    menuModeActive = true;
    userInterface->resetDiagnosticMode();
    // 触发一个假的 pendingMode 来促使 handleModeChange 保存 EEPROM 并解除当前模式
    switchToMode(MODE_NORMAL);
    menuModeActive = true; // 重新覆盖为 true 因为 switchToMode 设为 false
    Serial.println("[MENU] Returned to Main Menu");
    // 立即重绘菜单，消除旧界面残留
    userInterface->clearDisplay();
    userInterface->renderMenu(menuSystem.getCurrentNode(), menuSystem.getCursorIndex(), menuSystem.getScrollOffset());
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
    if (currentMode == MODE_CONFIG_DIAMETER) {
      Serial.println("[CONFIG] Saving configuration to EEPROM...");
      
      // 初始化EEPROM
      EEPROM.begin(512);
      
      // 定义EEPROM中存储直径范围的起始地址
      const int EEPROM_DIAMETER_RANGES_ADDR = 0;
      
      // 写入直径范围数据
      EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR, 0xAA); // 写入魔术字节
      for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2, sorter.getOutletMinDiameter(i));
        EEPROM.write(EEPROM_DIAMETER_RANGES_ADDR + 1 + i * 2 + 1, sorter.getOutletMaxDiameter(i));
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
    if (oldMode == MODE_CONFIG_DIAMETER && currentMode != MODE_CONFIG_DIAMETER) {
      // 重置配置处理类状态
      diameterConfigHandler.reset();
    }
    
    // 移除模式LED控制，LED现在只用于显示出口状态
    
    // 打印模式切换完成信息
      Serial.print("[DIAGNOSTIC] Mode switched to: ");
      Serial.println(getSystemModeName(currentMode));
    
    // 显示模式变化信息到OLED
    // userInterface->displayModeChange(currentMode); // Removed as per instruction
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
    versionInfo += "Tel: 133-0640-0990\n";
    versionInfo += "Boot Count: " + String(systemBootCount);
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
    int latestDiameter = allTrays->getTrayDiameter(0);
    
    // 调用专用显示方法
    userInterface->displayNormalModeDiameter(latestDiameter);
  }
}

// 检查掉电情况
void checkPowerLoss() {
    // 读取电源监视引脚（假设使用模拟输入）
    // 如果是数字引脚（例如Input Only），analogRead可能不工作，视硬件而定
    // Config定义了PIN_POWER_MONITOR = 34 (ADC1_CH6)
    int powerValue = analogRead(PIN_POWER_MONITOR);
    
    // 如果电压低于阈值（掉电检测）
    if (powerValue < POWER_LOSS_ADC_THRESHOLD) {
        Serial.println("!!! POWER LOSS DETECTED !!!");
        Serial.print("ADC Value: ");
        Serial.println(powerValue);
        Serial.println("Saving system state...");
        
        // 紧急保存托盘数据
        allTrays->saveToEEPROM(EEPROM_ADDR_TRAY_DATA);
        
        // 提交EEPROM（这是最耗时的步骤，希望电容能撑住）
        EEPROM.commit();
        
        Serial.println("System Saved. Halting.");
        
        // 进入死循环，防止低电压下的错误行为，直到断电
        while (true) {
            delay(10);
        }
    }
}

void loop() {
  // 优先检查掉电 (暂时禁用，因为目前是USB供电)
  // checkPowerLoss();
  
  // ===================================
  // ENCODER DIAGNOSTICS LOGGING
  // ===================================
  // 获取输入状态
  int debugEncoderDelta = userInterface->getEncoderDelta();
  bool debugBtnPressed = userInterface->isMasterButtonPressed();
  
  // ===================================

  // 如果在菜单模式中
  if (menuModeActive) {
      bool needsRefresh = false;

      // 检查返回动作 (长按返回上级菜单)
      if (userInterface->isMasterButtonLongPressed()) {
          if (menuSystem.getCurrentNode() != nullptr && menuSystem.getCurrentNode()->parent != nullptr) {
              menuSystem.handleInput(0, false); 
              needsRefresh = true;
          }
      }
      
      // 处理导航
      if (debugEncoderDelta != 0) {
          menuSystem.handleInput(debugEncoderDelta, false);
          needsRefresh = true;
      }
      
      // 处理点击选择
      if (debugBtnPressed) {
          menuSystem.handleInput(0, true);
          if (menuModeActive) needsRefresh = true; // 如果点击后仍在菜单中（如进入子菜单），则刷新
      }

      if (needsRefresh) {
          userInterface->renderMenu(menuSystem.getCurrentNode(), menuSystem.getCursorIndex(), menuSystem.getScrollOffset());
      }
  } 
  else {
      // 在功能/诊断模式中
      // 检查退出动作：现在改为短按即返回
      if (debugBtnPressed) {
          handleReturnToMenu();
          return;
      }
      
      uint32_t currentMs = millis(); 
      
      // 执行各模式特定的诊断/处理逻辑
      int delta = debugEncoderDelta; // 使用之前获取好的 delta
      
      switch (currentMode) {
          case MODE_NORMAL:
              if (delta != 0) {
                  normalModeSubmode = (normalModeSubmode + 1) % 2;
              }
              processNormalMode();
              sorter.run();
              break;
              
          case MODE_DIAGNOSE_ENCODER:
              if (delta != 0) encoderDiagnosticHandler.switchToNextSubMode();
              encoderDiagnosticHandler.update();
              break;
              
          case MODE_DIAGNOSE_SCANNER:
              if (delta != 0) scannerDiagnosticHandler.switchToNextSubMode();
              scannerDiagnosticHandler.update();
              break;
              
          case MODE_DIAGNOSE_OUTLET:
              // 独占模式：忽略编码器旋转，只通过按键返回菜单
              if (UserInterface::getInstance()->isMasterButtonPressed()) {
                  handleReturnToMenu();
              } else {
                  outletDiagnosticHandler.update(currentMs);
                  // 关键修复：诊断模式下也需要驱动物理输出和处理脉冲逻辑
                  sorter.run(); 
              }
              break;
              
          case MODE_CONFIG_DIAMETER:
              diameterConfigHandler.update();
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
  
  // 处理模式切换
  handleModeChange();
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

    default:
      return "Unknown Mode";
  }
}

