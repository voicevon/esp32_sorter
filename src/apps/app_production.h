#ifndef APP_PRODUCTION_H
#define APP_PRODUCTION_H

#include "app_base_diagnostic_handler.h"
#include "../user_interface/common/display_types.h"

class AppProduction : public AppBase {
private:
    int _subMode; // Tracks submodes in normal production if any

public:
    AppProduction();
    
    void begin() override;
    void update(uint32_t currentTime, bool btnPressed) override;
    void end() override;
    void captureSnapshot(DisplaySnapshot& snapshot) override;
};

#endif // APP_PRODUCTION_H
