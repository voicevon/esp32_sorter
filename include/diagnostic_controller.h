// 诊断器类定义文件
#ifndef DIAGNOSTIC_CONTROLLER_H
#define DIAGNOSTIC_CONTROLLER_H

#include <Arduino.h>
#include "pins.h"

// 系统工作模式定义
enum SystemMode {
  MODE_NORMAL = 0,       // 正常工作模式
  MODE_DEBUG_ENCODER = 1, // 编码器调试模式
  MODE_DEBUG_SCANNER = 2,  // 扫描仪调试模式
  MODE_DEBUG_DIVERTER = 3, // 分支器调试模式
  MODE_TEST = 4           // 测试模式
};

class DiagnosticController {
private:
  SystemMode currentMode;      // 当前工作模式
  SystemMode pendingMode;      // 待切换的工作模式
  bool modeChangePending;      // 模式切换标志
  
  // 按钮去抖参数
  const unsigned long DEBOUNCE_DELAY = 50; // 按钮去抖延迟时间(毫秒)
  unsigned long lastButtonPressTime;   // 上次按钮按下时间
  int lastButtonState;                 // 上次按钮状态
  
  // 闪烁控制变量
  unsigned long lastBlinkTime;          // 上次闪烁时间
  const unsigned long BLINK_INTERVAL = 500; // 闪烁间隔500ms
  
  // 更新LED状态显示
  void updateStatusLEDs();
  
  // 切换工作模式的内部实现
  void switchToNextMode();
  
public:
  // 构造函数
  DiagnosticController();
  
  // 初始化诊断器
  void initialize();
  
  // 更新诊断器状态（应在主循环中调用）
  void update();
  
  // 获取当前工作模式
  SystemMode getCurrentMode() const;
  
  // 检查是否有模式切换待处理
  bool isModeChangePending() const;
  
  // 应用待切换的工作模式
  void applyModeChange();
  
  // 通过外部命令切换到下一个模式
  void switchModeExternally();
  
  // 获取当前模式的字符串描述
  String getCurrentModeName() const;
};

#endif // DIAGNOSTIC_CONTROLLER_H