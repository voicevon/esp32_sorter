#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

// 应用程序类型定义
enum AppType {
  APP_PRODUCTION = 0,            // 正常生产模式
  APP_DIAG_ENCODER = 1,          // 诊断编码器模式
  APP_DIAG_SCANNER = 2,          // 诊断扫描仪模式
  APP_DIAG_OUTLET = 3,           // 诊断出口模式
  APP_CONFIG_DIAMETER = 4,       // 配置出口直径范围模式
  APP_VERSION_INFO = 5,          // 版本信息模式
  APP_DIAG_HMI = 6,              // 诊断HMI编码器模式
  APP_CONFIG_PHASE_OFFSET = 7    // 配置编码器零位偏移量
};

// 全局系统名称变量
extern String systemName;

// 全局变量 extern 声明
extern int normalModeSubmode;
extern bool hasVersionInfoDisplayed;
extern unsigned long systemBootCount;

#endif // MAIN_H