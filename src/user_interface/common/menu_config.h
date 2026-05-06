#ifndef MENU_CONFIG_H
#define MENU_CONFIG_H

#include <Arduino.h>
#include "user_interface/drv_oled_rotary/menu_system.h"

// 全局菜单实例
extern MenuSystem menuSystem;
extern bool menuModeActive;

// 初始化菜单树
void setupMenuTree();

#endif // MENU_CONFIG_H
