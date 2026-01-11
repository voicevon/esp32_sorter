#include "oled.h"
#include "encoder.h"
#include "sorter.h"
#include "main.h"

// 初始化静态实例指针
OLED* OLED::instance = nullptr;

// 外部实例声明
extern Encoder* encoder;
extern Sorter sorter;

// 私有构造函数实现
OLED::OLED() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {
  lastUpdateTime = 0;
  isTemporaryDisplayActive = false;
  temporaryDisplayStartTime = 0;
  temporaryDisplayDuration = 0;
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
  
  // 设置显示器可用标识
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
void OLED::update(SystemMode currentMode, uint8_t outletCount, Sorter* sorter) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
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
  if (currentMode == MODE_NORMAL) {
    // 正常模式：显示用户要求的数据
    display.setCursor(0, 12);
    display.setTextSize(1);
    
    // 第一行：最新直径
    display.print(F("Diameter: "));
    display.print(sorter->getLatestDiameter());
    display.println(F(" mm"));
    
    // 第二行：分拣速度（根/小时）
    display.print(F("Speed: "));
    display.print(sorter->getSortingSpeed());
    display.println(F(" /h"));
    
    // 第三行：已识别数量和托架数量
    display.print(F("Items: "));
    display.print(sorter->getIdentifiedCount());
    display.print(F(" | Trays: "));
    display.println(sorter->getTrayCount());
  } else {
    // 非正常模式：保持原有显示
    drawSystemInfo(currentMode);
    drawEncoderInfo();
    drawOutletInfo(outletCount);
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
  
  // 启动临时显示计时（1秒）
  isTemporaryDisplayActive = true;
  temporaryDisplayStartTime = millis();
  temporaryDisplayDuration = 1000;
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
void OLED::displayDiagnosticInfo(const String& info) {
  // 检查显示器是否可用
  if (!isDisplayAvailable) {
    return;
  }
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("Diagnostic"));
  display.println(F("----------------"));
  display.println(info);
  
  display.display();
  
  // 启动临时显示计时（1.5秒）
  isTemporaryDisplayActive = true;
  temporaryDisplayStartTime = millis();
  temporaryDisplayDuration = 1500;
}

// 绘制头部信息
void OLED::drawHeader() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("ESP32 Sorter"));
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
    default:
      display.println(F("Unknown"));
      break;
  }
}

// 绘制编码器信息
void OLED::drawEncoderInfo() {
  display.setCursor(0, 22);
  display.print(F("Pos: "));
  display.println(encoder->getCurrentPosition());
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