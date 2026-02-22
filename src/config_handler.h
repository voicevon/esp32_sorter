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
    handleEncoderInputs();
    if (userInterface->isMasterButtonPressed()) {
      switchToNextSubMode();
    }
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
  
  // 处理旋钮输入
  virtual void handleEncoderInputs() {
    int encDelta = userInterface->getEncoderDelta();
    if (encDelta != 0) {
      handleValueChange(encDelta);
    }
  }
  
  // 处理配置值变化（由子类实现）
  virtual void handleValueChange(int delta) = 0;
};

// 直径配置处理类
class DiameterConfigHandler : public ConfigHandler {
public:
  DiameterConfigHandler(UserInterface* ui, Sorter* s) : ConfigHandler(ui, s) {}
  
protected:
  void initializeMode() override;
  void handleSubModeChange() override;
  void handleValueChange(int delta) override;
};



#endif // CONFIG_HANDLER_H
