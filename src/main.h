#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

// 系统工作模式定义
enum SystemMode {
  MODE_NORMAL = 0,            // 正常工作模式
  MODE_DIAGNOSE_ENCODER = 1,  // 诊断编码器模式
  MODE_DIAGNOSE_SCANNER = 2,  // 诊断扫描仪模式
  MODE_DIAGNOSE_OUTLET = 3,   // 诊断出口模式
  MODE_CONFIG_DIAMETER = 4,    // 配置出口直径范围模式
  MODE_VERSION_INFO = 5,       // 版本信息模式
  MODE_DIAGNOSE_HMI = 6        // 诊断HMI编码器模式
};

// 全局系统名称变量
extern String systemName;

// 全局变量 extern 声明
extern int normalModeSubmode;
extern bool hasVersionInfoDisplayed;
extern unsigned long systemBootCount;

#endif // MAIN_H