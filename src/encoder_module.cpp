// 编码器模块实现文件
#include "encoder_module.h"

// 全局编码器对象实例
EncoderModule encoder;

// 构造函数
EncoderModule::EncoderModule() : 
  pulseCount(0), 
  revolutionCount(0), 
  lastPinA(0),
  targetAngle(-1),
  isTriggered(false),
  angleCallback(nullptr) {
}

// 初始化编码器模块
void EncoderModule::initialize() {
  // 设置引脚模式
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_PIN_Z, INPUT_PULLUP);
  
  // 记录初始A相状态
  lastPinA = digitalRead(ENCODER_PIN_A);
  
  // 重置计数
  pulseCount = 0;
  revolutionCount = 0;
  isTriggered = false;
  
  // 注册中断
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), handleEncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_Z), handleZeroInterrupt, RISING);
  
  Serial.println("编码器模块初始化完成");
}

// 获取当前总脉冲数
int32_t EncoderModule::getPulseCount() const {
  return pulseCount;
}

// 获取当前圈数
int32_t EncoderModule::getRevolutionCount() const {
  return revolutionCount;
}

// 获取当前角度（0-359度）
uint16_t EncoderModule::getCurrentAngle() const {
  // 计算当前圈内的脉冲数并转换为角度
  uint16_t currentPulse = getPulseInCurrentRevolution();
  return map(currentPulse, 0, ENCODER_PPR - 1, 0, 359);
}

// 获取当前圈内的脉冲数（0-199）
uint16_t EncoderModule::getPulseInCurrentRevolution() const {
  // 使用模运算获取当前圈内的脉冲数
  int32_t currentPulse = pulseCount % ENCODER_PPR;
  if (currentPulse < 0) {
    currentPulse += ENCODER_PPR;  // 处理负数情况
  }
  return static_cast<uint16_t>(currentPulse);
}

// 设置角度触发点和回调函数
void EncoderModule::setAngleTrigger(uint16_t angle, AngleCallback callback) {
  if (angle < 360) {
    targetAngle = angle;
    angleCallback = callback;
    isTriggered = false;  // 重置触发状态
    Serial.printf("设置角度触发点：%d度\n", angle);
  } else {
    Serial.println("错误：角度必须在0-359度范围内");
  }
}

// 获取触发状态
bool EncoderModule::getTriggerState() const {
  return isTriggered;
}

// 重置编码器计数
void EncoderModule::resetCount() {
  noInterrupts();  // 关闭中断
  pulseCount = 0;
  revolutionCount = 0;
  isTriggered = false;
  interrupts();    // 开启中断
  Serial.println("编码器计数已重置");
}

// 编码器中断处理函数 - 四状态解码
void EncoderModule::handleEncoderInterrupt() {
  uint8_t pinA = digitalRead(ENCODER_PIN_A);
  uint8_t pinB = digitalRead(ENCODER_PIN_B);
  
  // 四状态解码算法
  if (pinA != encoder.lastPinA) {
    // 根据A相和B相的关系判断旋转方向
    if (pinA == pinB) {
      // 逆时针旋转
      encoder.pulseCount--;
    } else {
      // 顺时针旋转
      encoder.pulseCount++;
    }
    
    // 更新上一次A相状态
    encoder.lastPinA = pinA;
    
    // 检查是否需要触发角度回调
    if (encoder.targetAngle != -1 && encoder.angleCallback != nullptr && !encoder.isTriggered) {
      uint16_t currentAngle = encoder.getCurrentAngle();
      // 简单的角度比较触发
      if (currentAngle >= encoder.targetAngle && currentAngle < encoder.targetAngle + 5) {  // 5度容差
        encoder.isTriggered = true;
        // 注意：在中断中不直接调用回调函数，只设置标志
        // 回调函数应该在主循环中检查并调用
      }
    }
  }
}

// 零位中断处理函数
void EncoderModule::handleZeroInterrupt() {
  // 检测旋转方向并更新圈数
  uint8_t pinA = digitalRead(ENCODER_PIN_A);
  
  // 根据A相在Z相触发时的状态判断旋转方向
  // 注意：这需要根据具体编码器的相位关系进行调整
  if (pinA == LOW) {  // 假设顺时针旋转时A相为低电平
    encoder.revolutionCount++;
  } else {
    encoder.revolutionCount--;
  }
  
  // 重置触发状态，允许在下一圈再次触发
  encoder.isTriggered = false;
  
  Serial.printf("检测到零位信号，当前圈数：%ld\n", encoder.revolutionCount);
}