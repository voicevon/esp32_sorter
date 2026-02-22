#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include <Arduino.h>
#include <vector>
#include <functional>

// 菜单项类型
enum MenuItemType {
    MENU_TYPE_SUBMENU,
    MENU_TYPE_ACTION,
    MENU_TYPE_BACK
};

class MenuNode;

// 菜单项动作回调
typedef std::function<void()> MenuAction;

class MenuItem {
public:
    String label;
    MenuItemType type;
    MenuNode* targetMenu; // 当 type 为 MENU_TYPE_SUBMENU 时，指向目标子菜单
    MenuAction action;    // 当 type 为 MENU_TYPE_ACTION 时，保存执行的动作

    // 默认构造
    MenuItem(String l, MenuItemType t, MenuNode* target = nullptr, MenuAction act = nullptr)
        : label(l), type(t), targetMenu(target), action(act) {}
};

class MenuNode {
public:
    String title;
    MenuNode* parent;
    std::vector<MenuItem> items;
    
    MenuNode(String t, MenuNode* p = nullptr) : title(t), parent(p) {}
    
    void addItem(MenuItem item) {
        items.push_back(item);
    }
};

class MenuSystem {
private:
    MenuNode* rootNode;
    MenuNode* currentNode;
    
    int cursorIndex;
    int scrollOffset;
    int maxVisibleItems; // OLED 屏幕一次能显示的菜单数
    
    // 灵敏度控制
    int pulsesPerStep;    // 每移动一个菜单项所需的编码器脉冲数
    int deltaAccumulator; // 累积的脉冲数
    
    void updateScroll();

public:
    MenuSystem(int visibleItems = 5);
    ~MenuSystem();

    // 构建菜单树
    void setRootMenu(MenuNode* root);
    
    // 输入驱动
    void handleInput(int encoderDelta, bool btnPressed);

    // 获取渲染数据
    MenuNode* getCurrentNode() { return currentNode; }
    int getCursorIndex() { return cursorIndex; }
    int getScrollOffset() { return scrollOffset; }
    int getMaxVisibleItems() { return maxVisibleItems; }
    
    // 设置每步所需的脉冲数（用于调整旋转手感）
    void setSensitivity(int pulses) { 
        if (pulses > 0) pulsesPerStep = pulses; 
        deltaAccumulator = 0; // 重置累加器防止突跳
    }
};

#endif // MENU_SYSTEM_H
