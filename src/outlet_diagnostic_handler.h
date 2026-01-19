#pragma once

#include "modular/outlet.h"
#include "main.h"
#include "user_interface/user_interface.h"

// 前向声明UserInterface和Sorter类，避免循环依赖
class UserInterface;
class Sorter;

/**
 * 出口诊断模式处理类
 * 负责处理出口诊断模式下的所有功能
 */
class OutletDiagnosticHandler {
private:
    // 出口对象指针数组（从Sorter类获取）
    Outlet* outlets[OUTLET_COUNT];
    
    // 出口数量定义 - 使用统一的OUTLET_COUNT宏
    static const int NUM_OUTLETS = OUTLET_COUNT;
    
    // 诊断模式状态变量
    unsigned long modeStartTime;
    unsigned long lastOutletTime;
    bool outletState;
    uint8_t currentOutlet;
    bool displayInitialized;
    int currentSubMode;
    
    // 私有方法：处理周期操作的公共逻辑
    void processCycleOperation(unsigned long currentTime, unsigned long interval, const String& testType);
    
public:
    /**
     * 构造函数
     */
    OutletDiagnosticHandler();
    
    /**
     * 初始化出口诊断模式
     */
    void initialize();
    
    /**
     * 设置UserInterface实例，用于显示诊断信息
     * @param ui UserInterface指针
     */
    void setUserInterface(UserInterface* ui);
    
    /**
     * 设置特定出口对象的指针
     * @param index 出口索引
     * @param outlet Outlet对象指针
     */
    void setOutlet(uint8_t index, Outlet* outlet);
    
    /**
     * 切换到下一个子模式
     */
    void switchToNextSubMode();
    
    /**
     * 主更新方法，处理所有诊断逻辑
     * @param currentTime 当前系统时间
     */
    void update(unsigned long currentTime);
    
    /**
     * 初始化诊断模式
     * @param currentTime 当前系统时间
     */
    void initializeDiagnosticMode(unsigned long currentTime);
    
private:
    // UserInterface指针，用于显示诊断信息
    UserInterface* userInterface;
};
