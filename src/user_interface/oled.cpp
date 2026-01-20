#include "oled.h"
// #include "display_data.h"已移除，不再需要

// 系统名称
String systemName = "Feng's AS-L9";

// DisplayData比较运算符已移除，改用更具体的数据比较方法

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
  
  // 初始化上一次显示的数据
  lastDisplayedMode = static_cast<SystemMode>(0);
  lastEncoderPosition = 0;
  lastSortingSpeedPerSecond = 0;
  lastSortingSpeedPerMinute = 0;
  lastSortingSpeedPerHour = 0;
  lastIdentifiedCount = 0;
  lastTransportedTrayCount = 0;
  lastLatestDiameter = 0;
  
  // 初始化扫描仪编码器值显示状态管理变量
  for (int i = 0; i < 4; i++) {
    lastRisingValues[i] = 0;
    lastFallingValues[i] = 0;
  }
  isFirstScannerDisplay = true;
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
// update方法已移除，改用功能专用方法

// 显示正常模式统计信息（子模式0）- 不再使用，已移至displayDashboard
// void OLED::displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
//   // 检查显示器是否可用
//   if (!isDisplayAvailable) {
//     return;
//   }
//   
//   // 检查临时显示是否活动，如果是则跳过常规更新
//   if (isTemporaryDisplayActive) {
//     checkTemporaryDisplayEnd();
//     return;
//   }
//   
//   // 清屏并设置基本显示参数
//   display.clearDisplay();
//   renderHeader();
//   display.setCursor(0, 12);
//   display.setTextSize(1);
//   
//   // 第一行：每秒多少根
//   display.print(F("Speed: "));
//   display.print(sortingSpeedPerSecond, 1);
//   display.println(F(" /s"));
//   
//   // 第二行：每分钟多少根
//   display.print(F("Speed: "));
//   display.print(sortingSpeedPerMinute);
//   display.println(F(" /m"));
//   
//   // 第三行：每小时多少根
//   display.print(F("Speed: "));
//   display.print(sortingSpeedPerHour);
//   display.println(F(" /h"));
//   
//   // 第四行：空格
//   display.println();
//   
//   // 第五行：已识别数量（右对齐）
//   String itemsText = "Items: " + String(identifiedCount);
//   int itemsTextWidth = itemsText.length() * 6; // 每个字符约6像素
//   int itemsCursorX = 128 - itemsTextWidth - 2; // 128是屏幕宽度，减去文本宽度和边距
//   if (itemsCursorX < 0) itemsCursorX = 0;
//   display.setCursor(itemsCursorX, 36);
//   display.print(itemsText);
//   
//   // 第六行：已运输托架数量（右对齐）
//   String traysText = "Transported: " + String(transportedTrayCount);
//   int traysTextWidth = traysText.length() * 6; // 每个字符约6像素
//   int traysCursorX = 128 - traysTextWidth - 2; // 128是屏幕宽度，减去文本宽度和边距
//   if (traysCursorX < 0) traysCursorX = 0;
//   display.setCursor(traysCursorX, 44);
//   display.print(traysText);
//   
//   // 显示内容
//   display.display();
// }

// 显示直径信息（功能专用方法）
void OLED::displayDiameter(int latestDiameter) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 检查临时显示是否活动，如果是则跳过常规更新
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  // 清屏并设置基本显示参数
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  
  // 第一行：最新直径（名义直径）
  display.print(F("Diameter: "));
  display.print(latestDiameter);
  display.println(F(" mm"));
  
  // 第二行：显示说明
  display.println(F("Nominal Diameter"));
  
  // 第三行：显示空行
  display.println();
  
  // 显示内容
  display.display();
}

// 显示正常模式（保留原方法以便兼容）
// displayNormalMode方法已移除，改用功能专用方法

// 显示速度统计信息
void OLED::displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 检查临时显示是否活动，如果是则跳过常规更新
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  // 清屏并设置基本显示参数
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  
  // 第一行：每秒多少根
  display.print(F("Speed: "));
  display.print(speedPerSecond);
  display.println(F(" /s"));
  
  // 第二行：每分钟多少根
  display.print(F("Speed: "));
  display.print(speedPerMinute);
  display.println(F(" /m"));
  
  // 第三行：每小时多少根
  display.print(F("Speed: "));
  display.print(speedPerHour);
  display.println(F(" /h"));
  
  // 第四行：空格
  display.println();
  
  // 第五行：已识别数量和托架数量
  display.print(F("Items: "));
  display.print(itemCount);
  display.print(F(" | Transported Trays: "));
  display.println(trayCount);
  
  // 显示内容
  display.display();
}

// 显示单个值（如直径、温度等）
void OLED::displaySingleValue(const String& label, int value, const String& unit) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 检查临时显示是否活动，如果是则跳过常规更新
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  // 清屏并设置基本显示参数
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  
  // 第一行：显示标签和值
  display.print(label);
  display.print(F(": "));
  display.print(value);
  if (!unit.isEmpty()) {
    display.print(F(" "));
    display.print(unit);
  }
  display.println();
  
  // 显示内容
  display.display();
}

// 显示位置信息
void OLED::displayPositionInfo(const String& title, int position, bool showOnlyOnChange) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 检查临时显示是否活动，如果是则跳过常规更新
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  // 清屏并设置基本显示参数
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  
  // 第一行：显示标题和位置
  display.print(title);
  display.print(F(": "));
  display.println(position);
  
  // 显示内容
  display.display();
}

// 显示诊断相关的两个值
void OLED::displayDiagnosticValues(const String& title, const String& value1, const String& value2) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 检查临时显示是否活动，如果是则跳过常规更新
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  // 清屏并设置基本显示参数
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  
  // 第一行：显示标题
  display.println(title);
  
  // 第二行：显示第一个值
  display.println(value1);
  
  // 第三行：显示第二个值
  display.println(value2);
  
  // 显示内容
  display.display();
}

// 显示多行文本信息
void OLED::displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 检查临时显示是否活动，如果是则跳过常规更新
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  // 清屏并设置基本显示参数
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  
  // 显示标题（如果有）
  if (!title.isEmpty()) {
    display.println(title);
  }
  
  // 显示第一行
  display.println(line1);
  
  // 显示第二行
  display.println(line2);
  
  // 显示第三行（如果有）
  if (!line3.isEmpty()) {
    display.println(line3);
  }
  
  // 显示内容
  display.display();
}

// 显示编码器诊断模式
// displayEncoderDiagnosticMode方法已移除，改用功能专用方法

// 显示出口诊断模式
// displayOutletDiagnosticMode方法已移除，改用功能专用方法

// 显示其他模式
// displayOtherModes方法已移除，改用功能专用方法

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
  
  // 重置上一次显示的数据，确保下一次更新能检测到模式变化
  lastEncoderPosition = 0;
  lastSortingSpeedPerSecond = 0;
  lastSortingSpeedPerMinute = 0;
  lastSortingSpeedPerHour = 0;
  lastIdentifiedCount = 0;
  lastTransportedTrayCount = 0;
  lastLatestDiameter = 0;
  
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

// 新方法：显示模式变化信息，不依赖SystemMode
void OLED::displayModeChange(const String& newModeName) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }

  // 重置上一次显示的数据，确保下一次更新能检测到模式变化
  lastEncoderPosition = 0;
  lastSortingSpeedPerSecond = 0;
  lastSortingSpeedPerMinute = 0;
  lastSortingSpeedPerHour = 0;
  lastIdentifiedCount = 0;
  lastTransportedTrayCount = 0;
  lastLatestDiameter = 0;

  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println(F("Mode Change"));
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print(F("New Mode: "));
  display.println(newModeName);

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
void OLED::renderHeader() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(systemName);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}



// 新方法：绘制状态栏，不依赖SystemMode
void OLED::renderStatusBar(const String& modeName) {
  display.setCursor(0, 12);
  display.print(F("Mode: "));
  display.println(modeName);
}

// 绘制编码器信息
void OLED::renderEncoderInfo(int encoderPosition) {
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
void OLED::renderOutletInfo(uint8_t outletCount) {
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
  
  // 根据子模式调用相应的专用显示方法
  switch (subMode) {
    case 0:
      displayOutletTestNormalOpen(outletCount, openOutlet);
      break;
    case 1:
      displayOutletTestNormalClosed(outletCount, openOutlet);
      break;
    case 2:
      displayOutletTestLifetime(outletCount, openOutlet);
      break;
    default:
      // 未知子模式，显示错误信息
      display.setCursor(0, 0);
      display.println("Unknown Submode");
  }
  
  display.display();
  
  // 设置诊断模式标志为true，保持显示内容
  isDiagnosticModeActive = true;
}

// 显示出口测试子模式0：Normally Open
void OLED::displayOutletTestNormalOpen(uint8_t outletCount, uint8_t openOutlet) {
  uint8_t maxDisplayOutlets = outletCount < 8 ? outletCount : 8;
  
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
}

// 显示出口测试子模式1：Normally Closed
void OLED::displayOutletTestNormalClosed(uint8_t outletCount, uint8_t openOutlet) {
  uint8_t maxDisplayOutlets = outletCount < 8 ? outletCount : 8;
  
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

// 显示出口测试子模式2：Servo Lifetime Test
void OLED::displayOutletTestLifetime(uint8_t outletCount, uint8_t cycleCount) {
  // 第一排：显示寿命测试标题
  display.setCursor(0, 0);
  display.println("Servo Lifetime");
  
  // 第二排：显示测试中
  display.setCursor(0, 10);
  display.println("Test Running");
  display.println("-----------");
  display.println();
  display.println();
  
  // 显示循环次数
  display.setCursor(0, 35);
  display.print("Cycle Count: ");
  display.println(cycleCount);
  
  // 显示循环状态
  display.setCursor(0, 45);
  display.print("Status: ");
  display.println("Cycling");
}

// 显示出口寿命测试专用图形
void OLED::displayOutletLifetimeTestGraphic(uint8_t outletCount, unsigned long cycleCount, bool outletState, int subMode) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  
  // 第一排：显示寿命测试标题
  display.setCursor(0, 0);
  display.println("Servo Lifetime");
  
  // 第二排：显示测试中
  display.setCursor(0, 10);
  display.println("Test Running");
  display.println("-----------");
  display.println();
  display.println();
  
  // 显示循环次数
  display.setCursor(0, 35);
  display.print("Cycle Count: ");
  display.println(cycleCount);
  
  // 显示所有出口状态
  display.setCursor(0, 45);
  display.print("All Outlets: ");
  display.println(outletState ? "Open" : "Closed");
  
  display.display();
  
  // 设置诊断模式标志为true，保持显示内容
  isDiagnosticModeActive = true;
}

// 显示扫描仪编码器值
void OLED::displayScannerEncoderValues(const int* risingValues, const int* fallingValues) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  // 检查编码器值是否发生变化
  bool dataChanged = false;
  if (isFirstScannerDisplay) {
    dataChanged = true;
    isFirstScannerDisplay = false;
  } else {
    // 比较上升沿编码器值
    for (int i = 0; i < 4; i++) {
      if (risingValues[i] != lastRisingValues[i] || fallingValues[i] != lastFallingValues[i]) {
        dataChanged = true;
        break;
      }
    }
  }
  
  // 只有当数据发生变化时才更新屏幕
  if (dataChanged) {
    display.clearDisplay();
    display.setTextSize(1);
    
    // 显示标题
    display.setCursor(0, 0);
    display.println("Scanner positions");
    display.println("-----------------");
    
    // 显示上升沿编码器值
    display.setCursor(0, 20);
    display.print("^:");
    for (int i = 0; i < 4; i++) {
      display.printf("%3d", risingValues[i]);
      if (i < 3) {
        display.print(" ");
      }
    }
    
    // 显示下降沿编码器值
    display.setCursor(0, 35);
    display.print("v:");
    for (int i = 0; i < 4; i++) {
      display.printf("%3d", fallingValues[i]);
      if (i < 3) {
        display.print(" ");
      }
    }
    
    display.display();
    
    // 更新上一次显示的编码器值
    for (int i = 0; i < 4; i++) {
      lastRisingValues[i] = risingValues[i];
      lastFallingValues[i] = fallingValues[i];
    }
  }
  
  // 设置诊断模式标志为true，保持显示内容
  isDiagnosticModeActive = true;
}

// 实现Display抽象基类的displayDashboard方法
void OLED::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    // 直接实现仪表盘显示逻辑，不依赖其他方法
    // 检查显示器是否可用
    if (!isDisplayAvailable) {
        return;
    }
    
    // 检查临时显示是否活动，如果是则跳过常规更新
    if (isTemporaryDisplayActive) {
        checkTemporaryDisplayEnd();
        return;
    }
    
    // 清屏并设置基本显示参数
    display.clearDisplay();
    renderHeader();
    display.setCursor(0, 12);
    display.setTextSize(1);
    
    // 第一行：每秒多少根
    display.print(F("Speed: "));
    display.print(sortingSpeedPerSecond, 1);
    display.println(F(" /s"));
    
    // 第二行：每分钟多少根
    display.print(F("Speed: "));
    display.print(sortingSpeedPerMinute);
    display.println(F(" /m"));
    
    // 第三行：每小时多少根
    display.print(F("Speed: "));
    display.print(sortingSpeedPerHour);
    display.println(F(" /h"));
    
    // 第四行：空格
    display.println();
    
    // 第五行：已识别数量（右对齐）
    String itemsText = "Items: " + String(identifiedCount);
    int itemsTextWidth = itemsText.length() * 6; // 每个字符约6像素
    int itemsCursorX = 128 - itemsTextWidth - 2; // 128是屏幕宽度，减去文本宽度和边距
    if (itemsCursorX < 0) itemsCursorX = 0;
    display.setCursor(itemsCursorX, 36);
    display.print(itemsText);
    
    // 第六行：已运输托架数量（右对齐）
    String traysText = "Transported: " + String(transportedTrayCount);
    int traysTextWidth = traysText.length() * 6; // 每个字符约6像素
    int traysCursorX = 128 - traysTextWidth - 2; // 128是屏幕宽度，减去文本宽度和边距
    if (traysCursorX < 0) traysCursorX = 0;
    display.setCursor(traysCursorX, 44);
    display.print(traysText);
    
    // 显示内容
    display.display();
}

// 兼容旧接口的displayNormalModeStats方法
void OLED::displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    // 直接调用新的功能专用方法
    displayDashboard(sortingSpeedPerSecond, sortingSpeedPerMinute, sortingSpeedPerHour, identifiedCount, transportedTrayCount);
}

// 兼容旧接口的displayNormalModeDiameter方法
void OLED::displayNormalModeDiameter(int latestDiameter) {
    // 直接调用新的功能专用方法
    displayDiameter(latestDiameter);
}

// 重置诊断模式显示标志
void OLED::resetDiagnosticMode() {
    isDiagnosticModeActive = false;
}