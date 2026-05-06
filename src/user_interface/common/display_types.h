#ifndef DISPLAY_TYPES_H
#define DISPLAY_TYPES_H

#include <Arduino.h>
#include "../../main.h"

/**
 * @struct DashboardSnapshot
 * @brief 正常生产/运行模式数据快照
 */
struct DashboardSnapshot {
    float sortingSpeedPerSecond;
    int sortingSpeedPerMinute;
    int sortingSpeedPerHour;
    int identifiedCount;
    int transportedTrayCount;
    int latestDiameter;
    int latestScanCount;
    int latestLengthLevel;
};

/**
 * @struct EncoderDiagSnapshot
 * @brief 编码器诊断与标定快照
 */
struct EncoderDiagSnapshot {
    int raw;
    int corrected;
    int logic;
    int zeroCount;
    int zeroCorrect;
    int zeroTotal;
    int offset;
};

/**
 * @struct ScannerDiagSnapshot
 * @brief 扫描仪/激光器诊断快照
 */
struct ScannerDiagSnapshot {
    uint8_t states;           // 4路激光实时挡光状态 (Bit 0-3)
    int sampleCount;          // 编码器当前周期内的采样点数
    uint8_t history[4][25];   // 4路激光历史200次采样数据位图 (4 * 25字节 = 800 bits)
};

/**
 * @struct OutletDiagSnapshot
 * @brief 出口动作诊断、配置、测试与寿命计数快照
 */
struct OutletDiagSnapshot {
    struct OutletItem {
        bool isOpen;          // 当前翻板是否打开
        float min;            // 出口分拣下限
        float max;            // 出口分拣上限
        uint8_t mask;         // 长度掩码
    } outlets[8];
    
    int activeOutletIndex;    // 当前选中用于诊断或编辑的出口索引 (0-7)
    int subMode;              // 子工作模式/视图状态
    uint32_t cycleCount;      // 阀体寿命测试当前计数值
};

/**
 * @struct DisplaySnapshot
 * @brief 全局显示快照顶级包装
 */
struct DisplaySnapshot {
    AppType currentMode;
    char activePage[32];      // 当前激活的HMI页面名称 (如 "dashboard", "diag_encoder" 等，避免使用 String 产生非平凡析构导致 union 无法编译)
    
    union {
        DashboardSnapshot dashboard;
        EncoderDiagSnapshot encoder;
        ScannerDiagSnapshot scanner;
        OutletDiagSnapshot outlet;
    } data;
};

#endif // DISPLAY_TYPES_H
