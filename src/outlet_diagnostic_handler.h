#pragma once

#include "outlet.h"
#include "main.h"

/**
 * 出口诊断模式处理类
 * 负责处理出口诊断模式下的所有功能
 */
class OutletDiagnosticHandler {
private:
    // 出口数量定义
    static const int NUM_OUTLETS = 10;
    
    // 出口对象数组
    Outlet outlets[NUM_OUTLETS];
    
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
     * 获取出口数量
     */
    uint8_t getOutletCount() const { return NUM_OUTLETS; }
    
    /**
     * 设置出口状态
     * @param outletIndex 出口索引
     * @param open 出口状态（true为打开，false为关闭）
     */
    void setOutletState(uint8_t outletIndex, bool open);
    
    /**
     * 处理出口相关任务
     */
    void processTasks();
};
