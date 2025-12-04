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
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  
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
  
  Serial.println("OLED display initialized");
}

// 更新显示内容
void OLED::update(SystemMode currentMode, uint8_t outletCount) {
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
  
  // 绘制系统信息
  drawSystemInfo(currentMode);
  
  // 绘制编码器信息
  drawEncoderInfo();
  
  // 绘制出口信息
  drawOutletInfo(outletCount);
  
  // 显示内容
  display.display();
}

// 显示模式变化信息
void OLED::displayModeChange(SystemMode newMode) {
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
    case MODE_DIAGNOSE_CONVEYOR:
      display.println(F("Conveyor"));
      break;
    case MODE_TEST:
      display.println(F("Test"));
      break;
    case MODE_TEST_RELOADER:
      display.println(F("Reloader"));
      break;
    default:
      display.println(F("Unknown"));
      break;
  }
  
  display.display();
  delay(1000); // 显示1秒
}

// 显示出口状态变化
void OLED::displayOutletStatus(uint8_t outletIndex, bool isOpen) {
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
  delay(500); // 显示0.5秒
}

// 显示诊断信息
void OLED::displayDiagnosticInfo(const String& info) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("Diagnostic"));
  display.println(F("----------------"));
  display.println(info);
  
  display.display();
  delay(1500); // 显示1.5秒
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
    case MODE_DIAGNOSE_CONVEYOR:
      display.println(F("Conv Diag"));
      break;
    case MODE_TEST:
      display.println(F("Test"));
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