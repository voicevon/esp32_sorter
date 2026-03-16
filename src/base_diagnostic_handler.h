#ifndef BASE_DIAGNOSTIC_HANDLER_H
#define BASE_DIAGNOSTIC_HANDLER_H

#include <Arduino.h>

/**
 * @brief 诊断处理器基类
 * 提供统一的生命周期管理接口：开始、更新、结束
 */
class BaseDiagnosticHandler {
public:
    virtual ~BaseDiagnosticHandler() {}

    // 进入诊断模式时的初始化逻辑
    virtual void begin() {}

    // 周期性更新逻辑
    virtual void update(unsigned long currentTime) = 0;

    // 退出诊断模式时的清理逻辑
    virtual void end() {}
};

#endif // BASE_DIAGNOSTIC_HANDLER_H
