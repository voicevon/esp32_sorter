#pragma once

#include "diameter_scanner.h"
#include "main.h"
#include "user_interface.h"
#include "encoder.h"

/**
 * 扫描仪诊断模式处理类
 * 负责处理扫描仪诊断模式下的所有功能
 */
class ScannerDiagnosticHandler {
private:
    // 直径扫描仪实例指针
    DiameterScanner* scanner;
    
    // 编码器实例指针
    Encoder* encoder;
    
    // 用户界面实例指针，用于显示操作
    UserInterface* userInterface;
    
    // 子模式管理变量
    int currentSubMode;      // 当前子模式
    int lastSubMode;         // 上一次子模式，用于检测子模式变化
    
    // 编码器值记录
    long risingEdgeEncoderValues[4];  // 上升沿编码器值
    long fallingEdgeEncoderValues[4]; // 下降沿编码器值
    
    // 传感器状态
    bool lastSensorStates[4];  // 上一次传感器状态
    bool risingEdges[4];       // 上升沿标志
    bool fallingEdges[4];      // 下降沿标志

public:
    /**
     * 构造函数
     */
    ScannerDiagnosticHandler();
    
    /**
     * 显示原始直径
     */
    void displayRawDiameters();
    
    /**
     * 编码器相位变化回调
     */
    void onPhaseChange(int phase);
    
    /**
     * 主更新方法，处理所有诊断逻辑
     */
    void update();
    
    /**
     * 切换到下一个子模式
     */
    void switchToNextSubMode();
    
    /**
     * 获取当前子模式
     */
    int getCurrentSubMode() const;
};
