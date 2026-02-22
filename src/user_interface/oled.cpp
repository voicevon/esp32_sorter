#include "oled.h"

// 外部引用系统名称（定义在main.cpp中）
extern String systemName;

// 初始化静态实例指针
OLED* OLED::instance = nullptr;

// 私有构造函数实现
OLED::OLED() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {
  lastUpdateTime = 0;
  isTemporaryDisplayActive = false;
  temporaryDisplayStartTime = 0;
  temporaryDisplayDuration = 0;
  isDiagnosticModeActive = false;
  isDisplayAvailable = false;
  
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
  Wire.setPins(PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.setClock(400000); // 尝试提速至 400KHz (Fast Mode)，进行压力测试
  
  // 初始化SSD1306显示器
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed - Display not available"));
    isDisplayAvailable = false;
    return;
  }
  
  // 设置显示器可用标识
  isDisplayAvailable = true;
  
  // 清屏
  display.clearDisplay();
  
  // 设置文本参数
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // 显示启动信息
  display.println(F("ESP32 Sorter"));
  display.println(F("I2C OLED"));
  display.println(F("IS ALIVE"));
  display.display();
  
  // 记录初始化时间
  lastUpdateTime = millis();
  
  Serial.println("OLED display (Adafruit) initialized successfully");
}

// 显示宽度信息
void OLED::displayDiameter(int latestDiameter) {
  if (!isDisplayAvailable) return;
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  display.print(F("Diameter: "));
  display.print(latestDiameter);
  display.println(F(" mm"));
  display.println(F("Nominal Diameter"));
  display.display();
}

// 显示速度统计
void OLED::displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) {
  if (!isDisplayAvailable) return;
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  display.print(F("Speed: ")); display.print(speedPerSecond); display.println(F(" /s"));
  display.print(F("Speed: ")); display.print(speedPerMinute); display.println(F(" /m"));
  display.print(F("Speed: ")); display.print(speedPerHour);   display.println(F(" /h"));
  display.println();
  display.print(F("Items: ")); display.print(itemCount);
  display.print(F(" | Trays: ")); display.println(trayCount);
  display.display();
}

// 显示单个数值
void OLED::displaySingleValue(const String& label, int value, const String& unit) {
  if (!isDisplayAvailable) return;
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  display.print(label);
  display.print(F(": "));
  display.print(value);
  if (!unit.isEmpty()) {
    display.print(F(" "));
    display.print(unit);
  }
  display.println();
  display.display();
}

// 显示位置信息
void OLED::displayPositionInfo(const String& title, int position, bool showOnlyOnChange) {
  if (!isDisplayAvailable) return;
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  display.print(title);
  display.print(F(": "));
  display.println(position);
  display.display();
}

// 显示诊断数值对
void OLED::displayDiagnosticValues(const String& title, const String& value1, const String& value2) {
  if (!isDisplayAvailable) return;
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  display.println(title);
  display.println(value1);
  display.println(value2);
  display.display();
}

// 显示多行文本
void OLED::displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3, const String& line4) {
  if (!isDisplayAvailable) return;
  if (isTemporaryDisplayActive) {
    checkTemporaryDisplayEnd();
    return;
  }
  
  display.clearDisplay();
  renderHeader();
  display.setCursor(0, 12);
  display.setTextSize(1);
  if (!title.isEmpty()) display.println(title);
  display.println(line1);
  display.println(line2);
  if (!line3.isEmpty()) display.println(line3);
  if (!line4.isEmpty()) display.println(line4);
  display.display();
}

// 显示模式变化信息
void OLED::displayModeChange(SystemMode newMode) {
  if (!isDisplayAvailable) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println(F("Mode Change"));
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print(F("New Mode: "));
  
  switch (newMode) {
    case MODE_NORMAL: display.println(F("Normal")); break;
    case MODE_DIAGNOSE_ENCODER: display.println(F("Encoder")); break;
    case MODE_DIAGNOSE_SCANNER: display.println(F("Scanner")); break;
    case MODE_DIAGNOSE_OUTLET: display.println(F("Outlet")); break;
    default: display.println(F("Unknown")); break;
  }
  
  display.display();
  isTemporaryDisplayActive = true;
  temporaryDisplayStartTime = millis();
  temporaryDisplayDuration = 2000;
}

// 字符串版本模式切换显示
void OLED::displayModeChange(const String& newModeName) {
  if (!isDisplayAvailable) return;

  // 彻底移除全屏提示逻辑，避免遮挡功能界面
  display.clearDisplay();
  display.display(); 
  isTemporaryDisplayActive = false; // 立即重置临时显示标志
}

// 显示出口状态变化
void OLED::displayOutletStatus(uint8_t outletIndex, bool isOpen) {
  if (!isDisplayAvailable) return;
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.print(F("Outlet "));
  display.println(outletIndex + 1);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print(F("Status: "));
  display.println(isOpen ? F("Open") : F("Closed"));
  display.display();
  
  isTemporaryDisplayActive = true;
  temporaryDisplayStartTime = millis();
  temporaryDisplayDuration = 500;
}

// 显示诊断详情
void OLED::displayDiagnosticInfo(const String& title, const String& info) {
  if (!isDisplayAvailable) return;
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(title);
  display.println(F("----------------"));
  display.println(info);
  display.display();
  isDiagnosticModeActive = true;
}

// 私有：渲染页眉
void OLED::renderHeader() {
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(systemName);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}

// 私有：渲染状态栏
void OLED::renderStatusBar(const String& modeName) {
  display.setCursor(0, 12);
  display.print(F("Mode: "));
  display.println(modeName);
}

// 私有：检查临时显示是否结束
void OLED::checkTemporaryDisplayEnd() {
  if (!isTemporaryDisplayActive) return;
  if (millis() - temporaryDisplayStartTime >= temporaryDisplayDuration) {
    isTemporaryDisplayActive = false;
  }
}

// 显示出口测试图形
void OLED::displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) {
  if (!isDisplayAvailable) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setTextSize(1);
  switch (subMode) {
    case 0: displayOutletTestNormalOpen(outletCount, openOutlet); break;
    case 1: displayOutletTestNormalClosed(outletCount, openOutlet); break;
    case 2: displayOutletTestLifetime(outletCount, openOutlet); break;
  }
  display.display();
  isDiagnosticModeActive = true;
}

// 出口测试子模式实现（简化版恢复）
void OLED::displayOutletTestNormalOpen(uint8_t outletCount, uint8_t openOutlet) {
  display.setCursor(0, 0); display.println("Outlet Test - NO");
  display.setCursor(0, 35);
  for (uint8_t i = 0; i < (outletCount < 8 ? outletCount : 8); i++) {
    if (i == openOutlet) display.print("  "); else { display.print(i + 1); display.print(" "); }
  }
}

void OLED::displayOutletTestNormalClosed(uint8_t outletCount, uint8_t openOutlet) {
  display.setCursor(0, 0); display.println("Outlet Test - NC");
  display.setCursor(0, 35);
  for (uint8_t i = 0; i < (outletCount < 8 ? outletCount : 8); i++) {
    if (i == openOutlet) { display.print(i + 1); display.print(" "); } else display.print("  ");
  }
}

void OLED::displayOutletTestLifetime(uint8_t outletCount, uint8_t cycleCount) {
  display.setCursor(0, 0); display.println("Servo Lifetime");
  display.setCursor(0, 35); display.print("Cycle: "); display.println(cycleCount);
}

// 寿命测试专用
void OLED::displayOutletLifetimeTestGraphic(uint8_t outletCount, unsigned long cycleCount, bool outletState, int subMode) {
  if (!isDisplayAvailable) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 0); display.println("Servo Lifetime");
  display.setCursor(0, 35); display.print("Cycle: "); display.println(cycleCount);
  display.setCursor(0, 45); display.print("State: "); display.println(outletState ? "Open" : "Closed");
  display.display();
}

// 扫描仪诊断显示
void OLED::displayScannerEncoderValues(const int* risingValues, const int* fallingValues) {
  if (!isDisplayAvailable) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 0); display.println("Scanner positions");
  display.setCursor(0, 20); display.print("^:");
  for (int i = 0; i < 4; i++) display.printf("%3d ", risingValues[i]);
  display.setCursor(0, 35); display.print("v:");
  for (int i = 0; i < 4; i++) display.printf("%3d ", fallingValues[i]);
  display.display();
}

// 菜单渲染
void OLED::renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) {
    if (!isDisplayAvailable || node == nullptr) return;
    if (isTemporaryDisplayActive) { checkTemporaryDisplayEnd(); return; }
    
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(node->title);
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    
    int startY = 14;
    int lineHeight = 10;
    for (int i = 0; i < 5; i++) {
        int idx = scrollOffset + i;
        if (idx >= (int)node->items.size()) break;
        int y = startY + (i * lineHeight);
        MenuItem& item = node->items[idx];
        
        if (idx == cursorIndex) {
            display.fillRect(0, y - 1, SCREEN_WIDTH, lineHeight, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        }
        display.setCursor(2, y);
        if (item.type == MENU_TYPE_SUBMENU) display.print(item.label + " >");
        else if (item.type == MENU_TYPE_BACK) display.print("< " + item.label);
        else display.print(item.label);
    }
    display.display();
}

// 仪表盘
void OLED::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    if (!isDisplayAvailable || isTemporaryDisplayActive) { checkTemporaryDisplayEnd(); return; }
    display.clearDisplay();
    renderHeader();
    display.setCursor(0, 12);
    display.printf("S: %.1f/s\n", sortingSpeedPerSecond);
    display.printf("S: %d/m\n", sortingSpeedPerMinute);
    display.printf("S: %d/h\n", sortingSpeedPerHour);
    display.setCursor(64, 36); display.printf("Items: %d", identifiedCount);
    display.setCursor(64, 46); display.printf("Trays: %d", transportedTrayCount);
    display.display();
}

void OLED::displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) {
    displayDashboard(sortingSpeedPerSecond, sortingSpeedPerMinute, sortingSpeedPerHour, identifiedCount, transportedTrayCount);
}

void OLED::displayNormalModeDiameter(int latestDiameter) {
    displayDiameter(latestDiameter);
}
// 重置诊断模式
void OLED::resetDiagnosticMode() {
  isDiagnosticModeActive = false;
  isFirstScannerDisplay = true;
}

// 清理屏幕
void OLED::clearDisplay() {
  if (!isDisplayAvailable) return;
  display.clearDisplay();
  display.display();
}
