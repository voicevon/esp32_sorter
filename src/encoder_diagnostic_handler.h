#pragma once

#include "modular/encoder.h"
#include "main.h"
#include "user_interface/user_interface.h"

/**
 * 编码器诊断模式处理类
 * 负责处理编码器诊断模式下的所有功能
 */
class EncoderDiagnosticHandler {
private:
    // 编码器实例指针
    Encoder* encoder;
    
    // 用户界面实例指针，用于显示操作
    UserInterface* userInterface;
    
    // 子模式管理变量
    int currentSubMode;      // 当前子模式
    bool subModeInitialized; // 子模式初始化标志

public:
    /**
     * 构造函数
     */
    EncoderDiagnosticHandler();
    
    /**
     * 初始化编码器诊断模式
     * @param ui UserInterface指针，用于显示诊断信息
     */
    void initialize(UserInterface* ui);
    
    /**
     * 切换到下一个子模式
     */
    void switchToNextSubMode();
    
    /**
     * 主更新方法，处理所有诊断逻辑
     */
    void update(unsigned long currentTime);
};
