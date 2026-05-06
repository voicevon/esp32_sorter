#ifndef APP_SCANNER_DIAG_H
#define APP_SCANNER_DIAG_H
#include "../modular/diameter_scanner.h"
#include "../main.h"
#include "../user_interface/user_interface.h"
#include "../modular/encoder.h"
#include "app_base_diagnostic_handler.h"

class AppScannerDiag : public AppBase {
private:
    DiameterScanner* scanner;
    Encoder* encoder;
    UserInterface* userInterface;
    int currentSubMode;
    int lastSubMode;
    
    // 编码器值记录
    long risingEdgeEncoderValues[4];  // 上升沿编码器值
    long fallingEdgeEncoderValues[4]; // 下降沿编码器值
    long minRisingEdgeValues[4];      // 有史以来最小的上升沿值
    long maxFallingEdgeValues[4];     // 有史以来最大的下降沿值
    int diameterDifferences[4];       // 直径差值（下降沿 - 上升沿，确保非负）
    bool hasCalculatedDifferences;    // 标记是否已经计算过差值
    
    // 传感器状态
    bool lastSensorStates[4];  // 上一次传感器状态
    bool risingEdges[4];       // 上升沿标志
    bool fallingEdges[4];      // 下降沿标志
    String lastIOStatus;       // 上一次IO状态字符串，用于避免重复输出
    int lastRawDiameters[4];   // 上一次原始直径值，用于避免重复输出
    int risingEdgeCounts[4];   // 上升沿计数器
    bool isFirstRun;           // 标记是否为该模式下第一次显示，用于串口表头重置
    
    // 子模式处理方法
    void handleIOStatusCheck();       // 子模式0：IO状态检查
    void handleEncoderValues();       // 子模式1：记录并显示传感器上升沿和下降沿的编码器值
    void handleRawDiameterDisplay();  // 子模式2：显示原始直径
    void handleWaveformDisplay();     // 子模式3：显示缓冲区的波形图

public:
    /**
     * 构造函数
     */
    AppScannerDiag();
    
    /**
     * 显示原始直径
     */
    void displayRawDiameters();
    
    /**
     * 初始化扫描仪诊断
     */
    void begin() override;

    /**
     * 主更新方法，处理所有诊断逻辑
     */
    void update(uint32_t currentTime, bool btnPressed) override;
    
    /**
     * 切换到下一个子模式
     */
    void switchToNextSubMode();
    void setSubMode(int mode);
    
    /**
     * 获取当前子模式
     */
    int getCurrentSubMode() const;
    void captureSnapshot(DisplaySnapshot& snapshot) override;
};
#endif // APP_SCANNER_DIAG_H
