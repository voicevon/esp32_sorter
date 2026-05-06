#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "../user_interface/user_interface.h"
#include "../modular/sorter.h"
#include "app_base.h"
#include <EEPROM.h>

// 配置处理基类
class AppConfig : public AppBase {
protected:
  UserInterface* userInterface;
  Sorter* sorter;
  bool modeInitialized;
  uint32_t lastRefreshMs;
  int currentSubMode;
  
public:
  AppConfig(UserInterface* ui, Sorter* s) : userInterface(ui), sorter(s), modeInitialized(false), currentSubMode(0) {}
  virtual ~AppConfig() {}
  
  // 初始化配置模式 (覆盖 AppBase)
  virtual void begin() override {
    modeInitialized = true;
    initializeMode();
  }
  
  // 更新逻辑由子类完全实现
  virtual void update(uint32_t currentMs, bool btnPressed) override = 0;
  
  void reset() {
    modeInitialized = false;
    currentSubMode = 0;
    lastRefreshMs = 0;
  }
  
protected:
  virtual void initializeMode() = 0;
  virtual void refreshDisplay() = 0;
  
  // 处理配置值变化（由子类实现）
  virtual void handleValueChange(int delta) = 0;
};

// 直径配置处理类 (高级多状态交互版)
class AppConfigDiameter : public AppConfig {
public:
  enum DiameterUIState {
      STATE_SELECTOR, // 列表滚动选择出口
      STATE_EDIT_MAX,  // 修改最大值
      STATE_EDIT_MIN,  // 修改最小值
      STATE_EDIT_LENGTH // 修改长度级别 (ANY,S,M,L)
  };

  AppConfigDiameter(UserInterface* ui, Sorter* s) : AppConfig(ui, s), uiState(STATE_SELECTOR) {}
  
  // 重置状态
  void reset() {
    AppConfig::reset();
    uiState = STATE_SELECTOR;
  }

protected:
  DiameterUIState uiState;
  int encoderAccumulator; // 用于实现自定义灵敏度分频

  void initializeMode() override;
  void handleValueChange(int delta) override;
  void refreshDisplay() override;
  // 覆盖按钮逻辑，实现状态跳转
  void update(uint32_t currentMs, bool btnPressed) override;
  void captureSnapshot(DisplaySnapshot& snapshot) override;
};

// 编码器零位偏移配置处理类
class AppConfigPhaseOffset : public AppConfig {
public:
  AppConfigPhaseOffset(UserInterface* ui, Sorter* s)
    : AppConfig(ui, s), editingOffset(0), encoderAccumulator(0) {}

  void reset() {
    AppConfig::reset();
    encoderAccumulator = 0;
  }

protected:
  int editingOffset;      // 当前正在编辑的偏移值（未保存）
  int encoderAccumulator; // 旋钮分频累加器

  void initializeMode() override;
  void handleValueChange(int delta) override;
  void refreshDisplay() override;
  void update(uint32_t currentMs, bool btnPressed) override;
  void captureSnapshot(DisplaySnapshot& snapshot) override;
};



#endif // APP_CONFIG_H
