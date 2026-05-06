#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>
#include "main.h"

class AppBase;

// 系统状态相关的全局变量
extern SystemMode currentMode;
extern SystemMode pendingMode;
extern bool modeChangePending;
extern unsigned long systemBootCount;
extern String firmwareVersion;
extern AppBase* activeApp;

// 切换模式辅助
void switchToMode(SystemMode mode);

// 处理模式切换
void handleModeChange();

// 检查掉电情况
void checkPowerLoss();

#endif // SYSTEM_MANAGER_H
