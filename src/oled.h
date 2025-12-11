#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "pins.h"
#include "main.h"

// SSD1306 I2C显示器引脚定义
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// I2C地址定义（SSD1306默认地址）
#define OLED_I2C_ADDRESS 0x3C

// 系统工作模式前向声明
enum SystemMode;

/**
 * @class OLED
 * @brief SSD1306 I2C显示器管理类
 * 
 * 该类使用单例模式实现，负责管理SSD1306 OLED显示器的初始化和显示功能
 * 显示系统状态、模式信息、出口状态、编码器数据和扫描仪数据等
 */
class OLED {
private:
  // 单例模式的私有构造函数
  OLED();
  
  // 静态实例指针
  static OLED* instance;
  
  // SSD1306显示器对象
  Adafruit_SSD1306 display;
  
  // 最后更新时间
  unsigned long lastUpdateTime;
  
  // 显示更新间隔（毫秒）
  const unsigned long UPDATE_INTERVAL = 500;
  
  // 临时显示状态管理
  bool isTemporaryDisplayActive;  // 是否正在显示临时信息
  unsigned long temporaryDisplayStartTime;  // 临时显示开始时间
  unsigned long temporaryDisplayDuration;  // 临时显示持续时间
  
  // 私有方法
  void drawHeader();
  void drawSystemInfo(SystemMode currentMode);
  void drawEncoderInfo();
  void drawOutletInfo(uint8_t outletCount);
  void checkTemporaryDisplayEnd();  // 检查临时显示是否结束
  
public:
  // 单例模式的获取实例方法
  static OLED* getInstance();
  
  // 初始化OLED显示器
  void initialize();
  
  // 更新显示内容
  void update(SystemMode currentMode, uint8_t outletCount);
  
  // 显示模式变化信息
  void displayModeChange(SystemMode newMode);
  
  // 显示出口状态变化
  void displayOutletStatus(uint8_t outletIndex, bool isOpen);
  
  // 显示诊断信息
  void displayDiagnosticInfo(const String& info);
};

#endif // OLED_H