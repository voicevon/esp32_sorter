// 诊断器类实现文件
#include "diagnostic_controller.h"
#include "sorter_controller.h"
#include "debug_module.h"
// 直径分类阈值在carriage_system.h中定义，不需要重复定义

DiagnosticController::DiagnosticController() {
  currentMode = MODE_NORMAL;
  pendingMode = MODE_NORMAL;
  modeChangePending = false;
  debugModule = nullptr;
  lastDataTime = 0;
  lastMoveTime = 0;
  showQueueStatus = false;
}

void DiagnosticController::initialize(DebugModule& module) {
  debugModule = &module;
  
  // 设置初始LED状态
  if (debugModule) {
    debugModule->setSystemMode(currentMode);
  }
  
  Serial.println("Diagnostic controller initialized");
}

void DiagnosticController::update() {
  // 检查DebugModule是否已初始化
  if (!debugModule) return;
  
  // 更新DebugModule状态
  debugModule->update();
  
  // 检查按钮1是否被按下（模式切换）
  if (debugModule->isButtonPressed(1)) {
    switchToNextMode();
    debugModule->clearButtonStates();
  }
}



void DiagnosticController::switchToNextMode() {
  // 计算下一个模式
  pendingMode = static_cast<SystemMode>((currentMode + 1) % 6); // 6种模式循环切换
  modeChangePending = true;
  
  // 打印模式切换请求信息
  Serial.print("[DIAGNOSTIC] Mode switch requested to: ");
  Serial.println(getCurrentModeName());
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
    
    // 更新DebugModule的LED显示
    if (debugModule) {
      debugModule->setSystemMode(currentMode);
    }
    
    // 打印模式切换完成信息
    Serial.print("[DIAGNOSTIC] Mode switched to: ");
    Serial.println(getCurrentModeName());
  }
}

void DiagnosticController::processCurrentMode(SorterController& sorterController) {
  unsigned long currentTime = millis();
  
  switch (currentMode) {
    case MODE_DIAGNOSE_ENCODER:
      // 诊断编码器模式
      if (currentTime - lastMoveTime > 1500) { // 1.5秒移动一次
        lastMoveTime = currentTime;
        sorterController.moveOnePosition();
        Serial.println("[DIAGNOSTIC] Simulated encoder movement");
      }
      break;
    
    case MODE_DIAGNOSE_SCANNER:
      // 诊断扫描仪模式
      if (currentTime - lastDataTime > 2000) { // 2秒生成一次数据
        lastDataTime = currentTime;
        // 随机生成一个直径数据（5.0mm - 25.0mm）
        float randomDiameter = 5.0f + random(0, 201) / 10.0f;
        sorterController.receiveDiameterData(randomDiameter);
        Serial.print("[DIAGNOSTIC] Generated random diameter: ");
        Serial.print(randomDiameter);
        Serial.println("mm");
      }
      break;
    
    case MODE_DIAGNOSE_DIVERTER:
      // 诊断分支器模式
      // 这里可以添加分支器测试的逻辑
      Serial.println("[DIAGNOSTIC] Diverter diagnostic mode active");
      break;
    
    case MODE_DIAGNOSE_CONVEYOR:
      // 诊断传输线模式
      if (currentTime - lastMoveTime > 1000) { // 1秒移动一次
        lastMoveTime = currentTime;
        sorterController.moveOnePosition();
        Serial.println("[DIAGNOSTIC] Conveyor diagnostic - simulated movement");
        // 打印传输线状态信息
        sorterController.displayCarriageQueue();
      }
      break;
    
    case MODE_TEST:
      // 测试模式
      if (currentTime - lastDataTime > 3000) { // 3秒生成一次数据
        lastDataTime = currentTime;
        // 随机生成一个直径数据（5.0mm - 25.0mm）
        float randomDiameter = 5.0f + random(0, 201) / 10.0f;
        sorterController.receiveDiameterData(randomDiameter);
        Serial.print("[TEST] Generated random diameter: ");
        Serial.print(randomDiameter);
        Serial.println("mm");
      }
      
      if (currentTime - lastMoveTime > 1000) { // 1秒移动一次
        lastMoveTime = currentTime;
        sorterController.moveOnePosition();
        Serial.println("[TEST] Simulated encoder movement");
      }
      
      // 显示队列状态（每2秒切换一次）
      if (currentTime % 2000 < 1000) {
        if (!showQueueStatus) {
          showQueueStatus = true;
          sorterController.displayCarriageQueue();
        }
      } else {
        showQueueStatus = false;
      }
      break;
    
    case MODE_NORMAL:
    default:
      // 正常模式下不需要特殊处理
      break;
  }
}

// 所有模式处理逻辑已移至processCurrentMode方法中



String DiagnosticController::getCurrentModeName() const {
  switch (currentMode) {
    case MODE_NORMAL:
      return "Normal";
    case MODE_DIAGNOSE_ENCODER:
      return "Diagnose Encoder";
    case MODE_DIAGNOSE_SCANNER:
      return "Diagnose Scanner";
    case MODE_DIAGNOSE_DIVERTER:
      return "Diagnose Diverter";
    case MODE_DIAGNOSE_CONVEYOR:
      return "Diagnose Conveyor";
    case MODE_TEST:
      return "Test";
    default:
      return "Unknown";
  }
}