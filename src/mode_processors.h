#ifndef MODE_PROCESSORS_H
#define MODE_PROCESSORS_H

#include <Arduino.h>
#include "main.h"

// 处理版本信息模式
void processVersionInfoMode();

// 处理正常模式
void processNormalMode();

// 获取系统模式名称
String getSystemModeName(SystemMode mode);

#endif // MODE_PROCESSORS_H
