#ifndef CONFIG_HANDLER_H
#define CONFIG_HANDLER_H

#include "user_interface/user_interface.h"
#include "modular/sorter.h"
#include "base_diagnostic_handler.h"
#include <EEPROM.h>

// 配置处理基类
class ConfigHandler : public BaseDiagnosticHandler {
protected:
  UserInterface* userInterface;
  Sorter* sorter;
  bool modeInitialized;
  uint32_t lastRefreshMs;
  int currentSubMode;
  
public:
  ConfigHandler(UserInterface* ui, Sorter* s) : userInterface(ui), sorter(s), modeInitialized(false), currentSubMode(0) {}
  virtual ~ConfigHandler() {}
  
  // 初始化配置模式 (覆盖 BaseDiagnosticHandler)
  virtual void begin() override {
    // 每次进入都强制标记为初始化，并执行具体模式的开始逻辑
    modeInitialized = true;
    initializeMode();
  }
  
  // 切换到下一个子模式
  virtual void switchToNextSubMode() {
    currentSubMode = (currentSubMode + 1) % getSubModeCount();
    if (currentSubMode == getSubModeCount() - 1) {
        // 预处理或显示提示，由具体实现决定
    }
    handleSubModeChange();
  }
  
  // 检查是否在退出状态并处理
  virtual bool checkExit() {
      if (currentSubMode == getSubModeCount() - 1) {
          handleReturnToMenu();
          return true;
      }
      return false;
  }

  // 更新配置模式 (覆盖 BaseDiagnosticHandler)
  virtual void update(uint32_t currentMs, bool btnPressed) override {
    begin();
    handleEncoderInputs();
    
    // 自愈刷新逻辑：即便没有物理动作，每 500ms 也强制同步一次 UI
    if (currentMs - lastRefreshMs >= 500) {
        lastRefreshMs = currentMs;
        refreshDisplay();
    }

    if (btnPressed) {
      if (!checkExit()) {
          lastRefreshMs = currentMs;
          switchToNextSubMode();
      }
    }
  }
  
  void reset() {
    modeInitialized = false;
    currentSubMode = 0;
    lastRefreshMs = 0;
  }
  
protected:
  virtual void initializeMode() = 0;
  virtual void refreshDisplay() = 0; // 为基类添加这一项
  virtual int getSubModeCount() { return 16; } 
  
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
  void refreshDisplay() override;
  int getSubModeCount() override { return NUM_OUTLETS * 2 + 1; } // 0-15为配置, 16为退出
};


// 伺服参数配置处理类
// 子模式: 0=加速时间 1=减速时间 2=正向转矩 3=反向转矩
class ServoConfigHandler : public ConfigHandler {
public:
  // EEPROM 地址（在 DiameterConfig 之后预留偏移）
  static constexpr int EEPROM_BASE_ADDR = 64; // 避免与直径配置(0-63)冲突
  static constexpr int EEPROM_MAGIC     = 0xBB;

  // 默认值
  static constexpr int DEFAULT_ACCEL_MS  = 200;  
  static constexpr int DEFAULT_DECEL_MS  = 200;  
  static constexpr int DEFAULT_MAX_SPEED = 2000; 
  static constexpr int DEFAULT_FWD_TRQ   = 100;  
  static constexpr int DEFAULT_REV_TRQ   = 100;  
  static constexpr int DEFAULT_SAFE_ON   = 0;     // 上电安全自锁模式 (PA35=0)

  ServoConfigHandler(UserInterface* ui, Sorter* s) : ConfigHandler(ui, s) {}

  // 从 EEPROM 加载参数（启动时调用）
  void loadFromEEPROM();

  // 将参数写入驱动器（永久地址）
  void applyToServo();

  // 获取参数值（供外部读取）
  int getAccelMs()  const { return _accelMs; }
  int getDecelMs()  const { return _decelMs; }
  int getMaxSpeed() const { return _maxSpeed; }
  int getFwdTorque()const { return _fwdTorque; }
  int getRevTorque()const { return _revTorque; }

protected:
  void initializeMode() override;
  void handleSubModeChange() override;
  void handleValueChange(int delta) override;
  void refreshDisplay() override;

public:
  void setEditParam(int subMode) {
      currentSubMode = subMode;
  }

private:
  int _accelMs   = DEFAULT_ACCEL_MS;
  int _decelMs   = DEFAULT_DECEL_MS;
  int _maxSpeed  = DEFAULT_MAX_SPEED;
  int _fwdTorque = DEFAULT_FWD_TRQ;
  int _revTorque = DEFAULT_REV_TRQ;
  int _safePowerUp = DEFAULT_SAFE_ON;

  void saveToEEPROM();
  String subModeName() const;
  int currentValue() const;
  void applyCurrentParam();
  int getSubModeCount() override { return 7; } // 0-5为参数, 6为退出
};

#endif // CONFIG_HANDLER_H
