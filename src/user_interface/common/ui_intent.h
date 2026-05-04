#ifndef UI_INTENT_H
#define UI_INTENT_H

#include <Arduino.h>

/**
 * @enum UIAction
 * @brief 抽象的用户交互动作类型
 * 
 * 将具体的硬件操作（如旋转编码器、点击触控屏、输入串口命令）
 * 映射为统一的逻辑动作。
 */
enum class UIAction {
    NONE,
    NAVIGATE_RELATIVE,  // 相对移动（如：编码器旋转、划动手势）
    NAVIGATE_PATH,      // 路径导航（如：点击特定菜单 ID）
    ACTIVATE,           // 激活/确认（如：短按旋钮、点击确定）
    CANCEL,             // 取消/退出
    BACK,               // 返回上一级
    SET_VALUE           // 直接设置值
};

/**
 * @struct UIIntent
 * @brief 承载用户意图的数据包
 */
struct UIIntent {
    UIAction action;
    int32_t  value;     // 携带的数值（如相对位移 delta，或特定菜单项索引）
    
    UIIntent() : action(UIAction::NONE), value(0) {}
    UIIntent(UIAction a, int32_t v = 0) : action(a), value(v) {}
    
    bool isValid() const { return action != UIAction::NONE; }
};

#endif // UI_INTENT_H
