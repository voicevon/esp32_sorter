#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../../config.h"
#include "../../main.h"
#include "../common/display.h"
#include "menu_system.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C

/**
 * @class OLED
 * @brief SSD1306 I2C显示器管理类
 */
class OLED : public Display {
private:
  OLED();
  static OLED* instance;
  Adafruit_SSD1306 display;
  bool isDisplayAvailable;
  uint32_t lastUpdateTime;
  const uint32_t UPDATE_INTERVAL = 100;

public:
  static OLED* getInstance();
  
  bool isAvailable() const override { return isDisplayAvailable; }
  void initialize() override;
  
  // 核心刷新接口
  void refresh(const DisplaySnapshot& snapshot) override;
  
  // 菜单渲染代理
  void renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) override;
  
  // 清理屏幕
  void clearDisplay() override;
};

#endif // OLED_H
