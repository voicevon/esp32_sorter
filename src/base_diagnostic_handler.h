#ifndef BASE_DIAGNOSTIC_HANDLER_H
#define BASE_DIAGNOSTIC_HANDLER_H

#include <Arduino.h>

// 全局模式返回辅助
extern void handleReturnToMenu();

/**
 * @brief 诊断处理器基类 
 * 提供统一的生命周期管理接口：开始、更新、结束
 */
class BaseDiagnosticHandler {
public:
    virtual ~BaseDiagnosticHandler() {}

    // 进入诊断模式时的初始化逻辑
    virtual void begin() {}

    // 定期更新接口，主逻辑
    virtual void update(uint32_t currentTime, bool btnPressed) = 0;

    // 退出诊断模式时的清理逻辑
    virtual void end() {}
};

#endif // BASE_DIAGNOSTIC_HANDLER_H
