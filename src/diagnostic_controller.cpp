// 诊断器类实现文件
#include "diagnostic_controller.h"

DiagnosticController::DiagnosticController() {
  currentMode = MODE_NORMAL;
  pendingMode = MODE_NORMAL;
  modeChangePending = false;
  lastButtonPressTime = 0;
  lastButtonState = HIGH;
  lastBlinkTime = 0;
}

void DiagnosticController::initialize() {
  // 初始化诊断器引脚
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP); // 模式按钮，上拉输入
  pinMode(STATUS_LED1_PIN, OUTPUT);      // 状态LED 1，输出
  pinMode(STATUS_LED2_PIN, OUTPUT);      // 状态LED 2，输出
  
  // 设置初始LED状态
  updateStatusLEDs();
  
  Serial.println("诊断器初始化完成");
}

void DiagnosticController::update() {
  // 检查按钮状态
  int currentButtonState = digitalRead(MODE_BUTTON_PIN);
  unsigned long currentTime = millis();
  
  // 按钮去抖处理
  if (currentButtonState != lastButtonState) {
    lastButtonPressTime = currentTime;
  }
  
  // 如果按钮状态稳定且为按下状态
  if (currentButtonState == LOW && 
      (currentTime - lastButtonPressTime) > DEBOUNCE_DELAY && 
      lastButtonState == HIGH) {
    // 切换系统模式
    switchToNextMode();
  }
  
  // 更新最后按钮状态
  lastButtonState = currentButtonState;
  
  // 特殊处理测试模式的LED闪烁效果
  if (currentMode == MODE_TEST) {
    if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
      lastBlinkTime = currentTime;
      // 切换LED状态
      digitalWrite(STATUS_LED1_PIN, !digitalRead(STATUS_LED1_PIN));
      digitalWrite(STATUS_LED2_PIN, !digitalRead(STATUS_LED2_PIN));
    }
  }
  // 如果模式改变但还未闪烁（非测试模式），更新LED显示
  else if (modeChangePending) {
    updateStatusLEDs();
  }
}

void DiagnosticController::updateStatusLEDs() {
  switch (currentMode) {
    case MODE_NORMAL:
      // 正常模式: LED1灭, LED2灭
      digitalWrite(STATUS_LED1_PIN, LOW);
      digitalWrite(STATUS_LED2_PIN, LOW);
      break;
    case MODE_DEBUG_ENCODER:
      // 编码器调试: LED1亮, LED2灭
      digitalWrite(STATUS_LED1_PIN, HIGH);
      digitalWrite(STATUS_LED2_PIN, LOW);
      break;
    case MODE_DEBUG_SCANNER:
      // 扫描仪调试: LED1灭, LED2亮
      digitalWrite(STATUS_LED1_PIN, LOW);
      digitalWrite(STATUS_LED2_PIN, HIGH);
      break;
    case MODE_DEBUG_DIVERTER:
      // 分支器调试: LED1亮, LED2亮
      digitalWrite(STATUS_LED1_PIN, HIGH);
      digitalWrite(STATUS_LED2_PIN, HIGH);
      break;
    case MODE_TEST:
      // 测试模式: LED1亮, LED2灭（初始状态，闪烁在update中处理）
      digitalWrite(STATUS_LED1_PIN, HIGH);
      digitalWrite(STATUS_LED2_PIN, LOW);
      lastBlinkTime = millis();
      break;
  }
}

void DiagnosticController::switchToNextMode() {
  // 计算下一个模式
  pendingMode = static_cast<SystemMode>((currentMode + 1) % 5);
  modeChangePending = true;
  
  // 打印模式切换信息
  Serial.print("[诊断器] 待切换到: ");
  switch (pendingMode) {
    case MODE_NORMAL:
      Serial.println("正常工作模式");
      break;
    case MODE_DEBUG_ENCODER:
      Serial.println("编码器调试模式");
      break;
    case MODE_DEBUG_SCANNER:
      Serial.println("扫描仪调试模式");
      break;
    case MODE_DEBUG_DIVERTER:
      Serial.println("分支器调试模式");
      break;
    case MODE_TEST:
      Serial.println("测试模式");
      break;
  }
}

SystemMode DiagnosticController::getCurrentMode() const {
  return currentMode;
}

bool DiagnosticController::isModeChangePending() const {
  return modeChangePending;
}

void DiagnosticController::applyModeChange() {
  if (modeChangePending) {
    currentMode = pendingMode;
    modeChangePending = false;
    
    // 更新LED显示
    updateStatusLEDs();
    
    // 打印模式切换完成信息
    Serial.print("[诊断器] 模式已切换为: ");
    Serial.println(getCurrentModeName());
  }
}

void DiagnosticController::switchModeExternally() {
  switchToNextMode();
}

String DiagnosticController::getCurrentModeName() const {
  switch (currentMode) {
    case MODE_NORMAL:
      return "正常工作模式";
    case MODE_DEBUG_ENCODER:
      return "编码器调试模式";
    case MODE_DEBUG_SCANNER:
      return "扫描仪调试模式";
    case MODE_DEBUG_DIVERTER:
      return "分支器调试模式";
    case MODE_TEST:
      return "测试模式";
    default:
      return "未知模式";
  }
}