#include "oled.h"
#include "display_data.h"

// 系统名称
String systemName = "Feng's AS-L9";

// 初始化静态实例指针
OLED* OLED::instance = nullptr;

// 私有构造函数实现
OLED::OLED() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {
  lastUpdateTime = 0;
  isTemporaryDisplayActive = false;
  temporaryDisplayStartTime = 0;
  temporaryDisplayDuration = 0;
  isDiagnosticModeActive = false;  // 初始化诊断模式标志
  isDisplayAvailable = false; // 初始化时假设显示器不可用
}

// 获取单例实例
OLED* OLED::getInstance() {
  if (instance == nullptr) {
    instance = new OLED();
  }
  return instance;
}

// 初始化OLED显示器
void OLED::initialize() {
  // 设置I2C引脚
  Wire.setPins(OLED_SDA_PIN, OLED_SCL_PIN);
  
  // 初始化SSD1306显示器
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed - Display not available"));
    isDisplayAvailable = false;
    return;
  }
  
  // 设置显示器可用标识133
  isDisplayAvailable = true;
  
  // 清屏
  display.clearDisplay();
  
  // 设置文本大小
  display.setTextSize(1);
  
  // 设置文本颜色
  display.setTextColor(SSD1306_WHITE);
  
  // 设置光标位置
  display.setCursor(0, 0);
  
  // 显示启动信息
  display.println(F("ESP32 Sorter"));
  display.println(F("Initializing..."));
  display.display();
  
  // 记录初始化时间
  lastUpdateTime = millis();
  
  Serial.println("OLED display initialized successfully");
}

// 更新显示内容
void OLED::update(const DisplayData& data) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 在MODE_DIAGNOSE_SCANNER、MODE_VERSION_INFO和MODE_DIAGNOSE_OUTLET模式下，保持诊断模式显示，不切换回常规显示
  if ((data.currentMode == MODE_DIAGNOSE_SCANNER || data.currentMode == MODE_VERSION_INFO || data.currentMode == MODE_DIAGNOSE_OUTLET) && isDiagnosticModeActive) {
    return;  // 跳过常规更新，保持当前显示内容
  }
  
  // 检查临时显示是否活动，如果是则跳过常规更新
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  // 检查是否需要更新
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime < UPDATE_INTERVAL) {
    return;
  }
  
  // 更新时间戳
  lastUpdateTime = currentTime;
  
  // 清屏
  display.clearDisplay();
  
  // 绘制头部信息
  drawHeader();
  
  // 根据不同模式显示不同内容
  if (data.currentMode == MODE_NORMAL) {
    // 正常模式：根据子模式显示不同信息
    display.setCursor(0, 12);
    display.setTextSize(1);
    
    if (data.normalSubMode == 0) {
      // 子模式0：统计信息
      // 第一行：每秒多少根
      display.print(F("Speed: "));
      display.print(data.sortingSpeedPerSecond);
      display.println(F(" /s"));
      
      // 第二行：每分钟多少根
      display.print(F("Speed: "));
      display.print(data.sortingSpeedPerMinute);
      display.println(F(" /m"));
      
      // 第三行：每小时多少根
      display.print(F("Speed: "));
      display.print(data.sortingSpeedPerHour);
      display.println(F(" /h"));
      
      // 第四行：空格
      display.println();
      
      // 第五行：已识别数量和托架数量
      display.print(F("Items: "));
      display.print(data.identifiedCount);
      display.print(F(" | Trays: "));
      display.println(data.trayCount);
    } else {
      // 子模式1：最新直径
      // 第一行：最新直径（名义直径）
      display.print(F("Diameter: "));
      display.print(data.latestDiameter);
      display.println(F(" mm"));
      
      // 第二行：显示说明
      display.println(F("Nominal Diameter"));
      
      // 第三行：显示空行
      display.println();
    }
  } else if (data.currentMode == MODE_DIAGNOSE_ENCODER) {
    // 编码器诊断模式：根据子模式显示不同信息
    display.setCursor(0, 12);
    display.setTextSize(1);
    
    if (data.encoderSubMode == 0) {
      // 子模式0：显示编码器位置（只在位置变化时更新）
      if (data.encoderPositionChanged) {
        display.print(F("Encoder Pos: "));
        display.println(data.encoderPosition);
      }
    } else {
      // 子模式1：显示相位变化信息（只在相位变化时更新）
      if (data.encoderPositionChanged) {
        display.print(F("Phase: "));
        display.println(data.encoderPosition);
      }
    }
  } else if (data.currentMode == MODE_DIAGNOSE_OUTLET) {
    // 出口测试模式：根据子模式显示不同信息
    display.setCursor(0, 12);
    display.setTextSize(1);
    
    if (data.outletSubMode == 0) {
      // 子模式0：轮巡降落（常态打开，偶尔闭合）
      display.println(F("Outlet Test:"));
      display.println(F("Mode 0: Open"));
    } else {
      // 子模式1：轮巡上升（常态闭合，偶尔打开）
      display.println(F("Outlet Test:"));
      display.println(F("Mode 1: Close"));
    }
  } else {
    // 非正常模式和非编码器诊断模式和非出口测试模式：保持原有显示
    drawSystemInfo(data.currentMode);
    drawEncoderInfo(data.encoderPosition);
    drawOutletInfo(data.outletCount);
  }
  
  // 显示内容
  display.display();
}

// 显示模式变化信息
void OLED::displayModeChange(SystemMode newMode) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 如果切换到MODE_DIAGNOSE_SCANNER、MODE_VERSION_INFO或MODE_DIAGNOSE_OUTLET模式，重置诊断模式标志
  if (newMode == MODE_DIAGNOSE_SCANNER || newMode == MODE_VERSION_INFO || newMode == MODE_DIAGNOSE_OUTLET) {
    isDiagnosticModeActive = false;
  }
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println(F("Mode Change"));
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print(F("New Mode: "));
  
  // 根据模式显示相应的名称
  switch (newMode) {
    case MODE_NORMAL:
      display.println(F("Normal"));
      break;
    case MODE_DIAGNOSE_ENCODER:
      display.println(F("Encoder"));
      break;
    case MODE_DIAGNOSE_SCANNER:
      display.println(F("Scanner"));
      break;
    case MODE_DIAGNOSE_OUTLET:
      display.println(F("Outlet"));
      break;
    case MODE_TEST_RELOADER:
      display.println(F("Reloader"));
      break;
    default:
      display.println(F("Unknown"));
      break;
  }
  
  display.display();
  
  // 启动临时显示计时（2秒）
  isTemporaryDisplayActive = true;
  temporaryDisplayStartTime = millis();
  temporaryDisplayDuration = 2000;
}

// 显示出口状态变化
void OLED::displayOutletStatus(uint8_t outletIndex, bool isOpen) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.print(F("Outlet "));
  display.println(outletIndex + 1);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print(F("Status: "));
  display.println(isOpen ? F("Open") : F("Closed"));
  
  display.display();
  
  // 启动临时显示计时（0.5秒）
  isTemporaryDisplayActive = true;
  temporaryDisplayStartTime = millis();
  temporaryDisplayDuration = 500;
}

// 显示诊断信息
void OLED::displayDiagnosticInfo(const String& title, const String& info) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(title);
  display.println(F("----------------"));
  display.println(info);
  
  display.display();
  
  // 在MODE_DIAGNOSE_SCANNER模式下，不使用临时显示机制
  // 而是设置诊断模式标志为true，保持显示内容
  isDiagnosticModeActive = true;
}

// 绘制头部信息
void OLED::drawHeader() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(systemName);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}

// 绘制系统信息
void OLED::drawSystemInfo(SystemMode currentMode) {
  display.setCursor(0, 12);
  display.print(F("Mode: "));
  
  // 根据模式显示相应的名称
  switch (currentMode) {
    case MODE_NORMAL:
      display.println(F("Normal"));
      break;
    case MODE_DIAGNOSE_ENCODER:
      display.println(F("Enc Diag"));
      break;
    case MODE_DIAGNOSE_SCANNER:
      display.println(F("Scan Diag"));
      break;
    case MODE_DIAGNOSE_OUTLET:
      display.println(F("Out Diag"));
      break;
    case MODE_TEST_RELOADER:
      display.println(F("Reload Test"));
      break;
    case MODE_VERSION_INFO:
      display.println(F("Version"));
      break;
    default:
      display.println(F("Unknown"));
      break;
  }
}

// 绘制编码器信息
void OLED::drawEncoderInfo(int encoderPosition) {
  display.setCursor(0, 22);
  display.print(F("Pos: "));
  display.println(encoderPosition);
}

// 检查临时显示是否结束
void OLED::checkTemporaryDisplayEnd() {
  if (!isTemporaryDisplayActive) return;
  
  unsigned long currentTime = millis();
  if (currentTime - temporaryDisplayStartTime >= temporaryDisplayDuration) {
    // 临时显示结束，重置状态
    isTemporaryDisplayActive = false;
    temporaryDisplayStartTime = 0;
    temporaryDisplayDuration = 0;
    // 临时显示结束后，下一次调用update()时会自动恢复常规显示
  }
}

// 绘制出口信息
void OLED::drawOutletInfo(uint8_t outletCount) {
  display.setCursor(0, 42);
  display.print(F("Outlets: "));
  display.println(outletCount);
  
  // 绘制出口状态指示（只显示出口编号，不显示状态）
  uint8_t maxDisplayOutlets = outletCount < 8 ? outletCount : 8;
  for (uint8_t i = 0; i < maxDisplayOutlets; i++) {
    display.setCursor(i * 16, 52);
    display.print(F("O"));
    display.print(i + 1);
  }
}

// 显示出口测试模式图形
void OLED::displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  
  uint8_t maxDisplayOutlets = outletCount < 8 ? outletCount : 8;
  
  // 根据子模式执行完全独立的显示逻辑
  if (subMode == 0) {
    // 子模式0：Normally Open
    // 第一排：显示出口测试标题
    display.setCursor(0, 0);
    display.println("Outlet Test");
    
    // 第二排：显示子模式
    display.print("   ");
    display.println("Normally Open");
    display.println("-----------");
    display.println();
    display.println();
    
    // 显示出口编号（最多8个）
    display.setCursor(0, 35);
    for (uint8_t i = 0; i < maxDisplayOutlets; i++) {
      if (i == openOutlet) {
        display.print("  ");  // 打开的出口位置显示空格
      } else {
        display.print(i + 1);
        display.print(" ");
      }
    }
    
    // 第二排：显示打开的出口数字，其他位置显示空格
    display.setCursor(0, 45);
    for (uint8_t i = 0; i < maxDisplayOutlets; i++) {
      if (i == openOutlet) {
        display.print(i + 1);  // 打开的出口显示数字
        display.print(" ");
      } else {
        display.print("  ");  // 其他位置显示空格
      }
    }
  } else {
    // 子模式1：Normally Closed
    // 第一排：显示出口测试标题
    display.setCursor(0, 0);
    display.println("Outlet Test");
    
    // 第二排：显示子模式
    display.print("  ");
    display.println("Normally Closed");
    display.println("-----------");
    display.println();
    display.println();
    
    // 在Normally Closed模式下，第一行和第二行互换显示
    // 第一行：显示打开的出口数字，其他位置显示空格
    display.setCursor(0, 35);
    for (uint8_t i = 0; i < maxDisplayOutlets; i++) {
      if (i == openOutlet) {
        display.print(i + 1);  // 打开的出口显示数字
        display.print(" ");
      } else {
        display.print("  ");  // 其他位置显示空格
      }
    }
    
    // 第二行：显示出口编号（最多8个）
    display.setCursor(0, 45);
    for (uint8_t i = 0; i < maxDisplayOutlets; i++) {
      if (i == openOutlet) {
        display.print("  ");  // 打开的出口位置显示空格
      } else {
        display.print(i + 1);
        display.print(" ");
      }
    }
  }
  

  
  display.display();
  
  // 设置诊断模式标志为true，保持显示内容
  isDiagnosticModeActive = true;
}

// 重置诊断模式显示标志
void OLED::resetDiagnosticMode() {
  isDiagnosticModeActive = false;
}