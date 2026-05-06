#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>
#include "main.h"

class AppBase;

// 系统状态相关的全局变量
extern AppType currentAppType;
extern AppType pendingAppType;
extern bool appTypeChangePending;
extern unsigned long systemBootCount;
extern String firmwareVersion;
extern AppBase* activeApp;

// 切换应用程序类型辅助
void switchToAppType(AppType appType);

// 处理应用程序切换
void handleAppTypeChange();

// 检查掉电情况
void checkPowerLoss();

#endif // SYSTEM_MANAGER_H
