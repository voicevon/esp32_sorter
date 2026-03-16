#ifndef ENCODER_DIAGNOSTIC_HANDLER_H
#define ENCODER_DIAGNOSTIC_HANDLER_H
#include "user_interface/user_interface.h"
#include "modular/encoder.h"
#include "base_diagnostic_handler.h"

class EncoderDiagnosticHandler : public BaseDiagnosticHandler {
private:
    Encoder* encoder;
    UserInterface* userInterface;
    int currentSubMode;
    bool subModeInitialized;

public:
    EncoderDiagnosticHandler();
    void initialize(UserInterface* ui);
    
    // 实现基类接口
    void begin() override;
    void update(unsigned long currentTime) override;
    void end() override;
    void switchToNextSubMode();
};
#endif // ENCODER_DIAGNOSTIC_HANDLER_H
