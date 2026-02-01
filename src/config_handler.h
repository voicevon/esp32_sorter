#ifndef CONFIG_HANDLER_H
#define CONFIG_HANDLER_H

#include "user_interface/user_interface.h"
#include "modular/sorter.h"

// 配置处理基类
class ConfigHandler {
protected:
  UserInterface* userInterface;
  Sorter* sorter;
  bool modeInitialized;
  int currentSubMode;
  
public:
  ConfigHandler(UserInterface* ui, Sorter* s) : userInterface(ui), sorter(s), modeInitialized(false), currentSubMode(0) {}
  virtual ~ConfigHandler() {}
  
  // 初始化配置模式
  virtual void initialize() {
    if (!modeInitialized) {
      modeInitialized = true;
      currentSubMode = 0;
      initializeMode();
    }
  }
  
  // 更新配置模式
  virtual void update() {
    initialize();
    // 这里不再自动检测长按，因为长按检测在handleSlaveButton()中统一处理
    handleButtonInputs();
  }
  
  // 重置配置模式
  void reset() {
    modeInitialized = false;
    currentSubMode = 0;
  }
  
protected:
  // 初始化具体模式（由子类实现）
  virtual void initializeMode() = 0;
  public:
  // 切换到下一个子模式
  virtual void switchToNextSubMode() {
    currentSubMode = (currentSubMode + 1) % 16; // 16个子模式
    handleSubModeChange();
  }
  
protected:
  
  // 处理子模式变化（由子类实现）
  virtual void handleSubModeChange() = 0;
  
  // 处理按钮输入
  virtual void handleButtonInputs() {
    handleIncreaseValue();
    handleDecreaseValue();
  }
  
  // 处理增加值（由子类实现）
  virtual void handleIncreaseValue() = 0;
  
  // 处理减少值（由子类实现）
  virtual void handleDecreaseValue() = 0;
};

// 直径配置处理类
class DiameterConfigHandler : public ConfigHandler {
public:
  DiameterConfigHandler(UserInterface* ui, Sorter* s) : ConfigHandler(ui, s) {}
  
protected:
  void initializeMode() override;
  void handleSubModeChange() override;
  void handleIncreaseValue() override;
  void handleDecreaseValue() override;
};

// 出口位置配置处理类
class OutletPosConfigHandler : public ConfigHandler {
public:
  OutletPosConfigHandler(UserInterface* ui, Sorter* s) : ConfigHandler(ui, s) {}
  
protected:
  void initializeMode() override;
  void handleSubModeChange() override;
  void handleIncreaseValue() override;
  void handleDecreaseValue() override;
};

#endif // CONFIG_HANDLER_H
