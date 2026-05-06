#ifndef APP_BASE_H
#define APP_BASE_H

#include <Arduino.h>

// 全局模式返回辅助
extern void handleReturnToMenu();

struct DisplaySnapshot;

/**
 * @brief 应用程序基类 
 * 提供统一的生命周期管理接口：开始、更新、结束
 */
class AppBase {
public:
    virtual ~AppBase() {}

    // 进入应用时的初始化逻辑
    virtual void begin() {}

    // 定期更新接口，主逻辑
    virtual void update(uint32_t currentTime, bool btnPressed) = 0;

    // 退出应用时的清理逻辑
    virtual void end() {}

    // 核心快照采集接口，各业务处理器将其数据组装进 DisplaySnapshot 中
    virtual void captureSnapshot(DisplaySnapshot& snapshot) {}
};

#endif // APP_BASE_H
