// 编码器模块头文件
#ifndef ENCODER_MODULE_H
#define ENCODER_MODULE_H

#include <Arduino.h>

// 编码器引脚定义
#define ENCODER_PIN_A 25
#define ENCODER_PIN_B 26
#define ENCODER_PIN_Z 27

// 编码器参数
#define ENCODER_PPR 200  // 每转脉冲数（根据用户需求设置为200）

// 角度触发回调函数类型
typedef void (*AngleCallback)();

class EncoderModule {
private:
  volatile int32_t pulseCount;   // 总脉冲计数
  volatile int32_t revolutionCount;  // 圈数计数
  volatile uint8_t lastPinA;     // 上一次A相状态
  
  // 角度触发配置
  int targetAngle;               // 目标触发角度（0-359度）
  bool isTriggered;              // 是否已触发
  AngleCallback angleCallback;   // 角度到达时的回调函数
  
public:
  EncoderModule();
  
  // 初始化编码器模块
  void initialize();
  
  // 获取当前总脉冲数
  int32_t getPulseCount() const;
  
  // 获取当前圈数
  int32_t getRevolutionCount() const;
  
  // 获取当前角度（0-359度）
  uint16_t getCurrentAngle() const;
  
  // 获取当前圈内的脉冲数（0-199）
  uint16_t getPulseInCurrentRevolution() const;
  
  // 设置角度触发点和回调函数
  void setAngleTrigger(uint16_t angle, AngleCallback callback);
  
  // 获取触发状态
  bool getTriggerState() const;
  
  // 重置编码器计数
  void resetCount();
  
  // 编码器中断处理函数
  static void handleEncoderInterrupt();
  
  // 零位中断处理函数
  static void handleZeroInterrupt();
};

// 全局编码器对象
extern EncoderModule encoder;

#endif // ENCODER_MODULE_H