#include "oled.h"

// 外部引用系统名称和版本（定义在main.cpp中）
extern String systemName;
extern String firmwareVersion;

#include "../modular/diameter_scanner.h"

// 初始化静态实例指针
OLED* OLED::instance = nullptr;

// 私有构造函数实现
// 使用 Wire1 以避开可能的系统级总线冲突
OLED::OLED() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1) {
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
  lastLatestScanCount = 0;
  
  // 初始化扫描仪编码器值显示状态管理变量
  for (int i = 0; i < 4; i++) {
    lastRisingValues[i] = 0;
    lastFallingValues[i] = 0;
  }
  isFirstScannerDisplay = true;
  
  // 初始化 I2C 统计变量
  i2cErrorCount = 0;
  lastI2CErrorCode = 0;
  i2cHealthy = true;
  lastRecoveryTime = 0;
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
  Serial.println(F("[OLED] Starting initialization..."));
  
  // 在初始化之前尝试恢复总线（针对 1.5 米线缆可能产生的挂死）
  Serial.println(F("[OLED] Attempting I2C bus recovery (20 pulses)..."));
  pinMode(PIN_OLED_SCL, OUTPUT);
  pinMode(PIN_OLED_SDA, INPUT_PULLUP);
  
  // 探测初始电平
  bool sda_init = digitalRead(PIN_OLED_SDA);
  bool scl_init = digitalRead(PIN_OLED_SCL);
  Serial.printf("[OLED] Pre-recovery levels: SDA=%d, SCL=%d\n", sda_init, scl_init);

  for (int i = 0; i < 20; i++) {
    digitalWrite(PIN_OLED_SCL, LOW);
    delayMicroseconds(10);
    digitalWrite(PIN_OLED_SCL, HIGH);
    delayMicroseconds(10);
  }
  
  // 探测恢复后电平
  pinMode(PIN_OLED_SCL, INPUT_PULLUP);
  delayMicroseconds(10);
  bool sda_post = digitalRead(PIN_OLED_SDA);
  bool scl_post = digitalRead(PIN_OLED_SCL);
  Serial.printf("[OLED] Post-recovery levels (as Inputs): SDA=%d, SCL=%d\n", sda_post, scl_post);
  
  // 使用 Wire1 重新设置引脚并以 100kHz 标准速率初始化
  Wire1.begin(PIN_OLED_SDA, PIN_OLED_SCL, 100000);
  Serial.printf("[OLED] Wire1.begin(SDA=%d, SCL=%d, 100kHz) done.\n", PIN_OLED_SDA, PIN_OLED_SCL);
  
  // 初始化SSD1306显示器
  Serial.println(F("[OLED] Calling display.begin() on Wire1..."));
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS, false, false)) { 
    Serial.printf("[OLED] SSD1306 initialization failed at address 0x%02X\n", OLED_I2C_ADDRESS);
    isDisplayAvailable = false;
    i2cHealthy = false;
    return;
  }
  
  Serial.println(F("[OLED] display.begin() successful."));
  
  // 执行总线扫描以二次确认
  Serial.println(F("[OLED] Scanning I2C bus (Wire1) at 1kHz (SDA=23, SCL=22)..."));
  byte error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire1.beginTransmission(address);
    error = Wire1.endTransmission();
    if (error == 0) {
      Serial.printf("[OLED] I2C device found at address 0x%02X\n", address);
      nDevices++;
    }
    delay(2); // 给 1.5 米线缆留出回收时间
  }
  
  if (nDevices == 0) {
    Serial.println(F("[OLED] No I2C devices found on bus (Wire1). Check physical connections."));
  }

  // 将速率锁定在 100kHz
  Wire1.setClock(100000); 
  Serial.println(F("[OLED] I2C Clock set to 100kHz."));
  
  // 设置显示器可用标识
  isDisplayAvailable = true;
  i2cHealthy = true;
  
  // 清屏
  display.clearDisplay();
  
  // 设置文本参数
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // 显示启动信息
  display.println(F("AS-L9 Sorter"));
  display.println(String(F("System ")) + firmwareVersion);
  display.println(F("Ready to Go"));
  
  Serial.println(F("[OLED] Performing initial safeDisplay()..."));
  bool success = safeDisplay(); 
  Serial.printf("[OLED] initial safeDisplay() result: %s\n", success ? "SUCCESS" : "FAILED");
  
  // 记录初始化时间
  lastUpdateTime = millis();
  
  Serial.println("OLED display (Adafruit) initialization sequence completed");
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
  safeDisplay();
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
  safeDisplay();
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
  safeDisplay();
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
  safeDisplay();
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
  safeDisplay();
}

// 显示多行文本
void OLED::displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3, const String& line4, const String& line5) {
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
  if (!line5.isEmpty()) display.println(line5);
  safeDisplay();
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
  
  safeDisplay();
  isTemporaryDisplayActive = true;
  temporaryDisplayStartTime = millis();
  temporaryDisplayDuration = 2000;
}

// 字符串版本模式切换显示
void OLED::displayModeChange(const String& newModeName) {
  if (!isDisplayAvailable) return;

  // 彻底移除全屏提示逻辑，避免遮挡功能界面
  display.clearDisplay();
  safeDisplay(); 
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
  safeDisplay();
  
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
  safeDisplay();
  isDiagnosticModeActive = true;
}

// 私有：渲染页眉
void OLED::renderHeader() {
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print(systemName);

  // 如果发生了 I2C 错误，在页眉显示计数器，辅助 1.5 米线缆的干扰排查
  if (i2cErrorCount > 0) {
      display.setCursor(SCREEN_WIDTH - 30, 0);
      display.print("E:");
      display.print(i2cErrorCount);
  }

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
void OLED::displayOutletTestGraphic(uint8_t outletCount, uint8_t selectedOutlet, bool isOpen, int subMode) {
  if (!isDisplayAvailable) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setTextSize(1);
  switch (subMode) {
    case 0: displayOutletTestNormalOpen(outletCount, selectedOutlet, isOpen); break;
    case 1: displayOutletTestNormalClosed(outletCount, selectedOutlet, isOpen); break;
    case 2: displayOutletTestLifetime(outletCount, selectedOutlet); break; // cycleCount reused as selectedOutlet
  }
  safeDisplay();
  isDiagnosticModeActive = true;
}

// 出口测试子模式实现（简化版恢复）
void OLED::displayOutletTestNormalOpen(uint8_t outletCount, uint8_t selectedOutlet, bool isOpen) {
  display.setCursor(0, 0); display.println("Outlet Test - Cycle");
  display.setCursor(0, 35);
  for (uint8_t i = 0; i < (outletCount < 8 ? outletCount : 8); i++) {
    if (i == selectedOutlet && isOpen) display.print("  "); 
    else { display.print(i + 1); display.print(" "); }
  }
}

void OLED::displayOutletTestNormalClosed(uint8_t outletCount, uint8_t selectedOutlet, bool isOpen) {
  display.setCursor(0, 0); display.println("Single Outlet Test");
  
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("Outlet Index: ");
  display.setTextSize(2);
  display.print(selectedOutlet + 1);
  
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("Status: ");
  display.setTextSize(2);
  if (isOpen) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(" OPEN ");
  } else {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.print("CLOSED");
  }
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setTextSize(1);
}

void OLED::displayOutletTestLifetime(uint8_t outletCount, uint8_t cycleCount) {
  display.setCursor(0, 0); display.println("Outlet Lifetime");
  display.setCursor(0, 35); display.print("Cycle: "); display.println(cycleCount);
}

// 寿命测试专用
void OLED::displayOutletLifetimeTestGraphic(uint8_t outletCount, uint32_t cycleCount, bool outletState, int subMode) {
  if (!isDisplayAvailable) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 0); display.println("Outlet Lifetime");
  display.setCursor(0, 35); display.print("Cycle: "); display.println(cycleCount);
  display.setCursor(0, 45); display.print("State: "); display.println(outletState ? "Open" : "Closed");
  safeDisplay();
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
  safeDisplay();
}

// 扫描仪波形图和原始计数合并显示
void OLED::displayScannerWaveform(DiameterScanner* scanner) {
  if (!isDisplayAvailable) return;
  display.clearDisplay();
  
  int samples = scanner->getSampleCount();
  int maxWaveWidth = SCREEN_WIDTH - 28; // 左侧给文字留28像素宽度 (大概4位数字空间)
  if (samples > maxWaveWidth) samples = maxWaveWidth;
  
  // 绘制 4 个通道的波形，每个通道占 15 像素高
  for (int ch = 0; ch < 4; ch++) {
    int yBase = 15 * ch + 12; // 底部基线
    
    // 绘制通道索引和脉冲计数 (例如: "0:45")
    display.setCursor(0, yBase - 8);
    display.setTextColor(SSD1306_WHITE);
    int count = scanner->getHighLevelPulseCount(ch);
    display.print(ch); 
    display.print(":"); 
    display.print(count);
    
    // 绘制波形点
    for (int x = 0; x < samples; x++) {
      uint8_t state = scanner->getSample(ch, x);
      int plotY = state ? (yBase - 8) : yBase; // 高电平在上，低电平在下
      display.drawPixel(28 + x, plotY, SSD1306_WHITE); // x 偏移 28
    }
  }
  
  safeDisplay();
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
    safeDisplay();
}

// 仪表盘 (128x64 整合版设计)
void OLED::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount, int latestDiameter, int latestScanCount) {
    if (!isDisplayAvailable || isTemporaryDisplayActive) { checkTemporaryDisplayEnd(); return; }
    
    display.clearDisplay();
    renderHeader();
    
    // -------------------------------------------------------------------
    // 左半区 (X: 0-63): 统计信息
    // -------------------------------------------------------------------
    display.setTextSize(1);
    display.setCursor(0, 14);
    display.print(F("Spd/s: ")); 
    display.print(sortingSpeedPerSecond, 1);
    
    display.setCursor(0, 24);
    display.print(F("Spd/m: ")); 
    display.print(sortingSpeedPerMinute);
    
    display.setCursor(0, 34);
    display.print(F("Items: ")); 
    display.print(identifiedCount);
    
    display.setCursor(0, 44);
    display.print(F("Trays: ")); 
    display.print(transportedTrayCount);

    // 绘制垂直分割线
    display.drawFastVLine(65, 12, 52, SSD1306_WHITE);

    // -------------------------------------------------------------------
    // 右半区 (X: 66-127): 实时扫描特写
    // -------------------------------------------------------------------
    
    // A. 直径大号字体显示 (核心视觉)
    display.setCursor(72, 14);
    display.setTextSize(3); // 降低到3以容纳3位数直径（如 120mm）
    if (latestDiameter > 0) {
        display.print(latestDiameter);
    } else {
        display.print("--");
    }
    
    display.setTextSize(1);
    display.setCursor(72, 38);
    display.print(F(" mm"));

    // B. 托架根数显示 (底部)
    display.setCursor(72, 52);
    display.print(F("Pcs: "));
    display.print(latestScanCount);
    
    safeDisplay();
}

void OLED::displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount, int latestDiameter, int latestScanCount) {
    displayDashboard(sortingSpeedPerSecond, sortingSpeedPerMinute, sortingSpeedPerHour, identifiedCount, transportedTrayCount, latestDiameter, latestScanCount);
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
  safeDisplay();
}

// 核心安全显示方法：处理 I2C 通信压力
bool OLED::safeDisplay() {
    if (!isDisplayAvailable) return false;

    // 尝试输出显示缓冲区
    display.display();

    // 针对 1.5 米线缆的通信稳定性：通过心跳探针检查 I2C 是否挂死
    Wire1.beginTransmission(OLED_I2C_ADDRESS);
    uint8_t error = Wire1.endTransmission();
    
    if (error != 0) {
        i2cErrorCount++;
        lastI2CErrorCode = error;
        i2cHealthy = false;
        
        // 详细日志输出
        Serial.printf("[OLED] safeDisplay Heartbeat FAILED! Error: %d, Count: %d\n", error, i2cErrorCount);
        
        // 如果错误是 2 (NACK on Address)，说明物理连接可能有瞬间抖动
        // 如果错误是 3 (NACK on Data) 或 4 (Other)，通常是总线卡死
        
        // 自动恢复机制：如果连续失败，尝试重新初始化 I2C 总线
        if (i2cErrorCount % 50 == 0) {
            Serial.printf("[OLED] Re-initializing Wire1 bus... (Internal Error: %d)\n", error);
            Wire1.begin(PIN_OLED_SDA, PIN_OLED_SCL, 100000);
        }
        return false;
    }

    i2cHealthy = true;
    return true;
}
