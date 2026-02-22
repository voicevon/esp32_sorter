#include "encoder_diagnostic_handler.h"

/**
 * 构造函数
 */
EncoderDiagnosticHandler::EncoderDiagnosticHandler() {
    // 获取编码器实例
    encoder = Encoder::getInstance();
    
    // 初始化子模式
    currentSubMode = 0;
    subModeInitialized = false;
    
    // 初始化用户界面指针为nullptr
    userInterface = nullptr;
}

/**
 * 初始化编码器诊断模式
 * @param ui UserInterface指针，用于显示诊断信息
 */
void EncoderDiagnosticHandler::initialize(UserInterface* ui) {
    userInterface = ui;
}

/**
 * 切换到下一个子模式
 */
void EncoderDiagnosticHandler::switchToNextSubMode() {
    // 切换到下一个子模式
    currentSubMode = (currentSubMode + 1) % 2;  // 2个子模式循环切换
    
    // 重置初始化标志，以便重新初始化子模式
    subModeInitialized = false;
    
    // 显示子模式切换信息
    String subModeName = currentSubMode == 0 ? "Position" : "Phase Change";
    Serial.println("[DIAGNOSTIC] Switch to Submode: " + subModeName);
    userInterface->displayDiagnosticInfo("Encoder Diag", "SubMode: " + subModeName);
}

/**
 * 主更新方法，处理所有诊断逻辑
 */
void EncoderDiagnosticHandler::update(unsigned long currentTime) {
    // 初始化子模式
    if (!subModeInitialized) {
        subModeInitialized = true;
        Serial.println("[DIAGNOSTIC] Submode: " + String(currentSubMode == 0 ? "Position" : "Phase Change"));
        Serial.println("[DIAGNOSTIC] Use slave button to switch submode");
    }
    
    // 获取编码器信息
    long rawCount = encoder->getRawCount();
    int encoderPosition = encoder->getCurrentPosition();
    long zeroCrossCount = encoder->getZeroCrossCount();
    bool positionChanged = encoder->hasPositionChanged();
    
    // 计算原始计数值（范围0-400）
    int rawPosition = rawCount % 400;
    if (rawPosition < 0) {
        rawPosition += 400;
    }
    
    // 获取强制清零次数和原始计数值
    int forcedZeroCount = encoder->getForcedZeroCount();
    long forcedZeroRawCount = encoder->getForcedZeroRawCount();
    
    // 使用新的显示方法，在OLED上用四行显示，冒号对齐
    // 第1行：原始计数值
    // 第2行：相位值
    // 第3行：空行
    // 第4行：强行置0次数与总清零次数的比例和最后清零时的数值
    String rawLine = "Raw:    " + String(rawPosition);
    String phaseLine = "Phase:  " + String(encoderPosition);
    String emptyLine = "";
    String combinedLine = "F: " + String(forcedZeroCount) + "/" + String(zeroCrossCount) + " Last: " + String(forcedZeroRawCount);
    userInterface->displayMultiLineText("Encoder", rawLine, phaseLine, combinedLine);
    

    
    // 重置位置变化标志
    encoder->resetPositionChanged();
}
