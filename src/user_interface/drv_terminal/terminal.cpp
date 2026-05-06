#include "terminal.h"

// 初始化静态实例指针
Terminal* Terminal::instance = nullptr;

// 私有构造函数实现
Terminal::Terminal() {
    Serial.begin(115200);
    delay(100);
    previousUpdateTime = 0;
}

// 单例模式获取实例
Terminal* Terminal::getInstance() {
    if (instance == nullptr) {
        instance = new Terminal();
    }
    return instance;
}

// 初始化串口终端
void Terminal::initialize() {
    Serial.println("Terminal initialized");
}

// 检查是否可以更新显示
bool Terminal::isUpdateReady() const {
    return millis() - previousUpdateTime >= UPDATE_INTERVAL;
}

// 核心快照刷新实现
void Terminal::refresh(const DisplaySnapshot& snapshot) {
    if (!isUpdateReady()) return;
    previousUpdateTime = millis();

    Serial.printf("\n[TERMINAL DISPLAY] Mode: %d | Page: %s\n", snapshot.currentMode, snapshot.activePage);
    switch (snapshot.currentMode) {
        case APP_PRODUCTION:
            Serial.printf("Dashboard: Speed=%.1f/s, %d/min, %d/h | Ident=%d, Trays=%d | Last=%d mm\n",
                snapshot.data.dashboard.sortingSpeedPerSecond,
                snapshot.data.dashboard.sortingSpeedPerMinute,
                snapshot.data.dashboard.sortingSpeedPerHour,
                snapshot.data.dashboard.identifiedCount,
                snapshot.data.dashboard.transportedTrayCount,
                snapshot.data.dashboard.latestDiameter);
            break;
        case APP_DIAG_ENCODER:
            Serial.printf("Encoder: Raw=%d, Corrected=%d, Logic=%d | ZCorrect=%d, ZError=%d | Offset=%d\n",
                snapshot.data.encoder.raw,
                snapshot.data.encoder.corrected,
                snapshot.data.encoder.logic,
                snapshot.data.encoder.zeroCount,
                snapshot.data.encoder.zeroTotal,
                snapshot.data.encoder.offset);
            break;
        case APP_DIAG_SCANNER:
            Serial.printf("Scanner: States=0x%02X | SampleCount=%d\n",
                snapshot.data.scanner.states,
                snapshot.data.scanner.sampleCount);
            break;
        case APP_DIAG_OUTLET:
        case APP_CONFIG_DIAMETER:
            Serial.printf("Outlets: ActiveOutlet=%d | SubMode=%d | Cycles=%d\n",
                snapshot.data.outlet.activeOutletIndex,
                snapshot.data.outlet.subMode,
                snapshot.data.outlet.cycleCount);
            break;
        default:
            break;
    }
}

// 清理屏幕
void Terminal::clearDisplay() {
    Serial.println("\n--- [Display Cleared] ---\n");
}

// 渲染菜单系统
void Terminal::renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) {
    // 串口可暂时选择不展示本地菜单，由 OLED 或 HMI 展示
}
