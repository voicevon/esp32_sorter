#ifndef INPUT_SOURCE_H
#define INPUT_SOURCE_H

#include "ui_intent.h"

/**
 * @class InputSource
 * @brief 输入设备抽象基类
 * 
 * 所有输入设备（旋钮、触控、串口等）必须实现此接口，
 * 以便 UserInterface 统一管理。
 */
class InputSource {
public:
    virtual ~InputSource() = default;

    /** 
     * @brief 驱动心跳
     * 在 UI 任务的主循环中被调用，用于处理硬件轮询、消抖等。
     */
    virtual void tick() = 0;

    /** 
     * @brief 检查是否有待处理的意图
     */
    virtual bool hasIntent() = 0;

    /** 
     * @brief 获取并清除当前意图（意图队列/标志位的消费）
     * @return 捕获到的 UIIntent
     */
    virtual UIIntent pollIntent() = 0;

    /**
     * @brief 获取设备名称（用于调试）
     */
    virtual String getName() const = 0;
};

#endif // INPUT_SOURCE_H
