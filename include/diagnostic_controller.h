// 诊断器类定义文件
#ifndef DIAGNOSTIC_CONTROLLER_H
#define DIAGNOSTIC_CONTROLLER_H

#include <Arduino.h>
#include "debug_module.h"

class SorterController;
class DebugModule;

class DiagnosticController {
private:
  SystemMode currentMode;      // 当前工作模式
  SystemMode pendingMode;      // 待切换的工作模式
  bool modeChangePending;      // 模式切换标志
  DebugModule* debugModule;    // 调试模块指针
  
  // 模式处理相关变量
  unsigned long lastDataTime;           // 上次生成数据时间
  unsigned long lastMoveTime;           // 上次移动时间
  bool showQueueStatus;                 // 队列状态显示标志
  
  // 切换工作模式的内部实现
  void switchToNextMode();
  
public:
  // 构造函数
  DiagnosticController();
  
  // 初始化诊断器
  void initialize(DebugModule& debugModule);
  
  // 更新诊断器状态（应在主循环中调用）
  void update();
  
  // 处理当前工作模式
  void processCurrentMode(SorterController& sorterController);
  
  // 获取当前工作模式
  SystemMode getCurrentMode() const;
  
  // 检查是否有模式切换待处理
  bool isModeChangePending() const;
  
  // 应用待切换的工作模式
  void applyModeChange();
  
  // 获取当前模式的字符串描述
  String getCurrentModeName() const;
};

#endif // DIAGNOSTIC_CONTROLLER_H