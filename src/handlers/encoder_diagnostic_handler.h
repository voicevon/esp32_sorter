#ifndef ENCODER_DIAGNOSTIC_HANDLER_H
#define ENCODER_DIAGNOSTIC_HANDLER_H
#include "../user_interface/user_interface.h"
#include "../modular/encoder.h"
#include "base_diagnostic_handler.h"

class EncoderDiagnosticHandler : public BaseDiagnosticHandler {
private:
    Encoder* encoder;
    UserInterface* userInterface;
    int currentSubMode;
    bool subModeInitialized;
    
    // 状态跟踪（用于本地化变化检测）
    long lastRawCount;
    int lastSpeed;
    long lastZeroCrossCount;
    unsigned long lastUIDisplayTime;
    const unsigned long UI_REFRESH_INTERVAL = 200; // 强制刷新间隔（毫秒）

public:
    EncoderDiagnosticHandler();
    void initialize(UserInterface* ui);
    
    // 实现基类接口
    void begin() override;
    void update(uint32_t currentTime, bool btnPressed) override;
    void end() override;
    void captureSnapshot(DisplaySnapshot& snapshot) override;
    void switchToNextSubMode();
};
#endif // ENCODER_DIAGNOSTIC_HANDLER_H
