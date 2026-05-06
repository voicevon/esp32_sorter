#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "../../main.h"  // 相对路径
#include "../../config.h"
#include "display_types.h"

class MenuNode;

/**
 * @class Display
 * @brief 显示设备抽象基类
 */
class Display {
public:
    virtual ~Display() = default;
    
    // 初始化显示设备
    virtual void initialize() = 0;
    
    // 检查显示设备是否可用
    virtual bool isAvailable() const = 0;

    // 核心接口：刷新系统快照数据
    virtual void refresh(const DisplaySnapshot& snapshot) = 0;
    
    // 渲染菜单系统 (保留以维持主菜单基础绘制)
    virtual void renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) = 0;
    
    // 清理屏幕
    virtual void clearDisplay() = 0;
};

#endif // DISPLAY_H
