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
  MODE_DIAGNOSE_POTENTIOMETER = 5, // 诊断电位器模式 (ADC Raw Test)
  MODE_SERVO_SPEED_ENCODER = 6,    // 伺服速度控制 (编码器)
  MODE_SERVO_SPEED_POTENTIOMETER = 7, // 伺服速度控制 (电位器)
  MODE_VERSION_INFO = 8,       // 版本信息模式
  MODE_DIAGNOSE_RS485 = 9,     // 诊断RS485模式
  MODE_DIAGNOSE_HMI = 10       // 诊断HMI编码器模式
};

// 全局系统名称变量
extern String systemName;

// 全局变量 extern 声明
extern int normalModeSubmode;
extern bool hasVersionInfoDisplayed;
extern unsigned long lastModbusSendTime;
extern unsigned long systemBootCount;

#endif // MAIN_H