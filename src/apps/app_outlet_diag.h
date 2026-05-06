#pragma once

#include "../modular/outlet.h"
#include "../main.h"
#include "../user_interface/user_interface.h"

// 前向声明UserInterface和Sorter类，避免循环依赖
class UserInterface;
class Sorter;

#include "app_base.h"

class AppOutletDiag : public AppBase {
private:
    Outlet* outlets[NUM_OUTLETS];
    
    // 诊断模式状态变量
    uint32_t modeStartTime;
    uint32_t lastOutletTime;
    bool outletState;
    uint8_t currentOutlet;
    bool displayInitialized;
    int currentSubMode;
    int lastSubMode;
    uint32_t lastUpdateTime;
    
    // 电磁铁寿命测试模式专用变量
    uint32_t cycleCount;  // 循环次数计数器
    
    // 私有方法：处理周期操作的公共逻辑
    void processCycleOperation(uint32_t currentTime, uint32_t interval, const String& testType);
    
public:
    /**
     * 构造函数
     */
    AppOutletDiag();
    
    /**
     * 初始化出口诊断模式
     * @param ui UserInterface指针，用于显示诊断信息
     */
    void initialize(UserInterface* ui);
    
    /**
     * 设置特定出口对象的指针
     * @param index 出口索引
     * @param outlet Outlet对象指针
     */
    void setOutlet(uint8_t index, Outlet* outlet);
    
    /**
     * 设置特定子模式
     * @param mode 子模式索引
     */
    void setSubMode(int mode);

    /**
     * 处理编码器输入 (用于单点测试模式切换选中出口)
     * @param delta 编码器变化量 (+1 or -1)
     */
    void handleEncoderInput(int delta);

    /**
     * 获取当前子模式
     */
    int getSubMode() const { return currentSubMode; }
    
    // 实现基类接口
    void begin() override;
    void update(uint32_t currentTime, bool btnPressed) override;
    void end() override;
    void captureSnapshot(DisplaySnapshot& snapshot) override;

    
private:
    // UserInterface指针，用于显示诊断信息
    UserInterface* userInterface;
    
    // 私有初始化方法
    void initializeDiagnosticMode(uint32_t currentTime);
};
