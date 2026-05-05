#include "rs485_touch_screen.h"
#include "../../config.h"

Rs485TouchScreen* Rs485TouchScreen::getInstance() {
    static Rs485TouchScreen instance;
    return &instance;
}

Rs485TouchScreen::Rs485TouchScreen() : _serial(&Serial2), _state(STATE_IDLE), _txTimestamp(0) {}

void Rs485TouchScreen::initialize() {
    pinMode(PIN_HMI_485_EN, OUTPUT);
    digitalWrite(PIN_HMI_485_EN, LOW); // Receive mode
    _serial->begin(HMI_MODBUS_BAUD, SERIAL_8N2, PIN_HMI_485_RX, PIN_HMI_485_TX);
    _rxBuffer.reserve(512);
    Serial.println("[Rs485TouchScreen] Initialized on Serial2");
}

uint8_t Rs485TouchScreen::calculateCRC8(const char* data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void Rs485TouchScreen::sendPayload(const String& jsonStr) {
    uint8_t crc = calculateCRC8(jsonStr.c_str(), jsonStr.length());
    String outStr = "$" + jsonStr + "*";
    if (crc < 0x10) outStr += "0";
    outStr += String(crc, HEX);
    outStr += "\n";
    outStr.toUpperCase();

    digitalWrite(PIN_HMI_485_EN, HIGH); // TX mode
    _serial->print(outStr);
    _serial->flush();
    digitalWrite(PIN_HMI_485_EN, LOW);  // RX mode

    _txTimestamp = millis();
    _state = STATE_WAITING_ACK;
    _rxBuffer = ""; // clear buffer for ack
}

void Rs485TouchScreen::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount, int latestDiameter, int latestScanCount, int latestLengthLevel) {
    if (_state != STATE_IDLE) {
        return; 
    }

    StaticJsonDocument<512> doc;
    doc["type"] = "dashboard";
    JsonObject data = doc.createNestedObject("data");
    data["speed"] = sortingSpeedPerSecond;
    data["yield"] = identifiedCount;
    data["capacity"] = sortingSpeedPerHour;
    data["diameter"] = latestDiameter;

    String jsonStr;
    serializeJson(doc, jsonStr);
    
    sendPayload(jsonStr);
}

void Rs485TouchScreen::tick() {
    if (_state == STATE_WAITING_ACK) {
        if (millis() - _txTimestamp > 100) { // 100ms timeout
            _state = STATE_IDLE;
            _rxBuffer = "";
            return;
        }
        
        while (_serial->available()) {
            char c = _serial->read();
            if (c == '\n') {
                processLine(_rxBuffer);
                _rxBuffer = "";
                _state = STATE_IDLE; // Received complete frame, go back to IDLE
                break; // Process one line per tick is enough
            } else {
                _rxBuffer += c;
                if (_rxBuffer.length() > 500) {
                    _rxBuffer = "";
                }
            }
        }
    }
}

void Rs485TouchScreen::processLine(const String& line) {
    String cleanLine = line;
    cleanLine.trim();
    if (!cleanLine.startsWith("$")) return;
    int starIdx = cleanLine.lastIndexOf('*');
    if (starIdx < 1) return;

    String jsonStr = cleanLine.substring(1, starIdx);
    String crcHexStr = cleanLine.substring(starIdx + 1);

    uint8_t calcCrc = calculateCRC8(jsonStr.c_str(), jsonStr.length());
    uint8_t recvCrc = (uint8_t) strtol(crcHexStr.c_str(), NULL, 16);

    if (calcCrc != recvCrc) {
        Serial.printf("[Rs485TouchScreen] CRC mismatch! Calc:%02X Recv:%02X\n", calcCrc, recvCrc);
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    if (error) return;

    // Parse events (if any) and push to intent queue
    if (doc.containsKey("events")) {
        JsonArray events = doc["events"];
        for (JsonObject ev : events) {
            String cmd = ev["cmd"] | "";
            int params = ev["params"] | 0;
            
            if (cmd == "tare") {
                _intentQueue.push(UIIntent(UIAction::SET_VALUE, params));
            } else if (cmd == "menu_click") {
                _intentQueue.push(UIIntent(UIAction::NAVIGATE_PATH, params));
            }
        }
    }
}

bool Rs485TouchScreen::hasIntent() {
    return !_intentQueue.empty();
}

UIIntent Rs485TouchScreen::pollIntent() {
    if (_intentQueue.empty()) return UIIntent(UIAction::NONE);
    UIIntent intent = _intentQueue.front();
    _intentQueue.pop();
    return intent;
}
