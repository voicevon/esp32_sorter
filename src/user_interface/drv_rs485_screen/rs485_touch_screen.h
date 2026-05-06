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
    void refresh(const DisplaySnapshot& snapshot) override;
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
    std::queue<String> _txQueue;

    void processLine(const String& line);
    uint8_t calculateCRC8(const char* data, size_t len);
    void sendPayload(const String& jsonStr);
    String _slavePage; // Tracks current page on Slave
    long _lastRawCount = 0;
};

#endif // RS485_TOUCH_SCREEN_H
