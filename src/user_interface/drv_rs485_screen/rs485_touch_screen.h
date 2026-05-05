#ifndef RS485_TOUCH_SCREEN_H
#define RS485_TOUCH_SCREEN_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <queue>
#include "../common/display.h"
#include "../common/input_source.h"

enum Rs485CommState {
    STATE_IDLE,
    STATE_WAITING_ACK
};

class Rs485TouchScreen : public Display, public InputSource {
public:
    static Rs485TouchScreen* getInstance();
    
    // Display interface
    void initialize() override;
    bool isAvailable() const override { return true; }
    void renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) override {}
    void displayModeChange(SystemMode newMode) override {}
    void displayModeChange(const String& newModeName) override {}
    void displayOutletStatus(uint8_t outletIndex, bool isOpen) override {}
    void displayDiagnosticInfo(const String& title, const String& info) override {}
    void displayOutletTestGraphic(uint8_t outletCount, uint8_t selectedOutlet, bool isOpen, int subMode) override {}
    void displayOutletLifetimeTestGraphic(uint8_t outletCount, uint32_t cycleCount, bool outletState, int subMode) override {}
    void displayScannerEncoderValues(const int* risingValues, const int* fallingValues) override {}
    void displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount, int latestDiameter, int latestScanCount, int latestLengthLevel = 0) override;
    void displayDiameter(int latestDiameter) override {}
    void displayNormalModeDiameter(int latestDiameter) override {}
    void displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount, int latestDiameter, int latestScanCount) override {}
    void displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) override {}
    void displaySingleValue(const String& label, int value, const String& unit) override {}
    void displayPositionInfo(const String& title, int position, bool showOnlyOnChange) override {}
    void displayDiagnosticValues(const String& title, const String& value1, const String& value2) override {}
    void displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3 = "", const String& line4 = "", const String& line5 = "") override {}
    void displayConfigEdit(const String& title, int maxV, int minV, uint8_t targetMode, int activeField) override {}
    void resetDiagnosticMode() override {}
    void clearDisplay() override {}

    // InputSource interface
    void tick() override; // Polling loop
    bool hasIntent() override;
    UIIntent pollIntent() override;
    String getName() const override { return "RS485_TouchScreen"; }

private:
    Rs485TouchScreen();
    ~Rs485TouchScreen() = default;

    HardwareSerial* _serial;
    String _rxBuffer;
    
    Rs485CommState _state;
    uint32_t _txTimestamp;
    uint32_t _frameCounter = 0; // 帧序列计数
    
    std::queue<UIIntent> _intentQueue;

    void processLine(const String& line);
    uint8_t calculateCRC8(const char* data, size_t len);
    void sendPayload(const String& jsonStr);
};

#endif // RS485_TOUCH_SCREEN_H
