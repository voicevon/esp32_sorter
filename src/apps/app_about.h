#ifndef APP_ABOUT_H
#define APP_ABOUT_H

#include "app_base.h"

class AppAbout : public AppBase {
public:
    AppAbout();
    void begin() override;
    void update(uint32_t currentTime, bool btnPressed) override;
    void end() override;
    void captureSnapshot(DisplaySnapshot& snapshot) override;
};

#endif // APP_ABOUT_H
