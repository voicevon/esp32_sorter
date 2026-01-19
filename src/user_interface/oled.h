#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "modular/pins.h"
#include "main.h"
#include "user_interface/display.h"  // 包含Display抽象基类头文件
// #include "display_data.h"已移除，不再需要

// SSD1306 I2C显示器引脚定义
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// I2C地址定义（SSD1306默认地址）
#define OLED_I2C_ADDRESS 0x3C

// 系统工作模式前向声明
enum SystemMode;

// DisplayData比较运算符已移除，改用更具体的数据比较方法

/**
 * @class OLED
 * @brief SSD1306 I2C显示器管理类
 * 
 * 该类使用单例模式实现，负责管理SSD1306 OLED显示器的初始化和显示功能
 * 显示系统状态、模式信息、出口状态、编码器数据和扫描仪数据等
 */
class OLED : public Display {
private:
  // 单例模式的私有构造函数
  OLED();
  
  // 静态实例指针
  static OLED* instance;
  
  // SSD1306显示器对象
  Adafruit_SSD1306 display;
  
  // 显示器是否可用的标识
  bool isDisplayAvailable;
  
  // 最后更新时间
  unsigned long lastUpdateTime;
  
  // 显示更新间隔（毫秒）
  const unsigned long UPDATE_INTERVAL = 500;
  
  // 临时显示状态管理
  bool isTemporaryDisplayActive;  // 是否正在显示临时信息
  unsigned long temporaryDisplayStartTime;  // 临时显示开始时间
  unsigned long temporaryDisplayDuration;  // 临时显示持续时间
  
  // 诊断模式显示状态管理
  bool isDiagnosticModeActive;  // 是否处于诊断模式显示状态
  
  // 扫描仪编码器值显示状态管理
  int lastRisingValues[4];  // 存储上一次显示的上升沿编码器值
  int lastFallingValues[4];  // 存储上一次显示的下降沿编码器值
  bool isFirstScannerDisplay;  // 指示是否是第一次显示扫描仪编码器值
  
  // 存储上一次显示的数据，用于检测变化
  SystemMode lastDisplayedMode;  // 上一次显示的模式
  int lastEncoderPosition;  // 上一次编码器位置
  int lastSortingSpeedPerSecond;  // 上一次显示的每秒速度
  int lastSortingSpeedPerMinute;  // 上一次显示的每分钟速度
  int lastSortingSpeedPerHour;  // 上一次显示的每小时速度
  int lastIdentifiedCount;  // 上一次显示的已识别数量
  int lastTransportedTrayCount;  // 上一次显示的已输送托盘数量
  int lastLatestDiameter;  // 上一次显示的最新直径
  
  // 私有方法
  void drawHeader();
  void drawSystemInfo(SystemMode currentMode); // 旧方法，待移除
  void drawStatusBar(const String& modeName); // 新方法，不依赖SystemMode
  void drawEncoderInfo(int encoderPosition);
  void drawOutletInfo(uint8_t outletCount);
  void checkTemporaryDisplayEnd();  // 检查临时显示是否结束
  
  // 模式专用显示方法已移除，改用功能专用方法
  
public:
  // 单例模式的获取实例方法
  static OLED* getInstance();
  
  // 检查显示器是否可用
  bool isAvailable() const { return isDisplayAvailable; }
  
  // 初始化OLED显示器
  void initialize();
  
  // 更新显示内容 - 已移除
  
  // 显示模式变化信息
  void displayModeChange(SystemMode newMode); // 旧方法，待移除
  void displayModeChange(const String& newModeName); // 新方法，不依赖SystemMode
  
  // 显示出口状态变化
  void displayOutletStatus(uint8_t outletIndex, bool isOpen);
  
  // 显示诊断信息
  void displayDiagnosticInfo(const String& title, const String& info);
  
  // 显示出口测试模式图形
    void displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode);
    
    // 显示扫描仪编码器值
    void displayScannerEncoderValues(const int* risingValues, const int* fallingValues);
    
  // 正常模式专用显示方法
  void displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount);
  void displayNormalModeDiameter(int latestDiameter);
  
  // 实现Display抽象基类的displayDashboard方法
  void displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount);
  
  // 新增功能专用显示方法
  void displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount);
  void displaySingleValue(const String& label, int value, const String& unit);
  void displayPositionInfo(const String& title, int position, bool showOnlyOnChange);
  void displayDiagnosticValues(const String& title, const String& value1, const String& value2);
  void displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3 = "");
  
  // 重置诊断模式显示标志（用于切换出MODE_DIAGNOSE_SCANNER模式时）
  void resetDiagnosticMode();
};

#endif // OLED_H