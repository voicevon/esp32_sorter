#include "menu_system.h"

MenuSystem::MenuSystem(int visibleItems) {
    maxVisibleItems = visibleItems;
    rootNode = nullptr;
    currentNode = nullptr;
    cursorIndex = 0;
    scrollOffset = 0;
    
    // 初始化灵敏度参数
    pulsesPerStep = 1;    // 默认1个脉冲代表1个菜单项，如果太灵敏可以改为 2 或 4
    deltaAccumulator = 0;
}

MenuSystem::~MenuSystem() {
    // 可选：在此处实现树节点的递归释放，或者由调用方管理生命周期
}

void MenuSystem::setRootMenu(MenuNode* root) {
    rootNode = root;
    currentNode = root;
    cursorIndex = 0;
    scrollOffset = 0;
}

void MenuSystem::updateScroll() {
    if (currentNode == nullptr || currentNode->items.empty()) return;
    
    if (cursorIndex < scrollOffset) {
        scrollOffset = cursorIndex;
    } else if (cursorIndex >= scrollOffset + maxVisibleItems) {
        scrollOffset = cursorIndex - maxVisibleItems + 1;
    }
}

void MenuSystem::handleInput(int encoderDelta, bool btnPressed) {
    if (currentNode == nullptr) return;

    // --- 旋钮处理：引入累积与分频逻辑 ---
    if (encoderDelta != 0 && !currentNode->items.empty()) {
        deltaAccumulator += encoderDelta;
        
        // 计算实际要移动的步数（基于灵敏度设置）
        int steps = deltaAccumulator / pulsesPerStep;
        
        if (steps != 0) {
            // 更新索引
            cursorIndex += steps;
            
            // 消耗掉对应的累积值（保留由于分频产生的余数）
            deltaAccumulator %= pulsesPerStep;
            
            // 边界限制
            int maxIndex = (int)currentNode->items.size() - 1;
            if (cursorIndex < 0) cursorIndex = 0;
            if (cursorIndex > maxIndex) cursorIndex = maxIndex;
            
            // 调试输出：观察菜单项目是如何跳转的
            Serial.printf("[MENU] Delta: %d, Acc: %d, Steps: %d, New Index: %d/%d\n", 
                          encoderDelta, deltaAccumulator, steps, cursorIndex, maxIndex);
            
            updateScroll();
        }
    }

    if (btnPressed) {
        MenuItem& selectedItem = currentNode->items[cursorIndex];
        
        if (selectedItem.type == MENU_TYPE_SUBMENU && selectedItem.targetMenu != nullptr) {
            currentNode = selectedItem.targetMenu;
            cursorIndex = 0;
            scrollOffset = 0;
        } else if (selectedItem.type == MENU_TYPE_BACK && currentNode->parent != nullptr) {
            currentNode = currentNode->parent;
            cursorIndex = 0;
            scrollOffset = 0;
        } else if (selectedItem.type == MENU_TYPE_ACTION && selectedItem.action != nullptr) {
            selectedItem.action();
        }
    }
}
