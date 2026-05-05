#include "rs485_touch_screen.h"
#include "../../config.h"
#include "../../modular/encoder.h"
#include "../../modular/diameter_scanner.h"
#include "../../modular/sorter.h"

extern Sorter sorter;

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

    digitalWrite(PIN_HMI_485_EN, HIGH); // TX mode
    _serial->print(outStr);
    _serial->flush();
    digitalWrite(PIN_HMI_485_EN, LOW);  // RX mode
    
    // Clear any local echo or noise that occurred during TX
    while (_serial->available()) {
        _serial->read();
    }

    _txTimestamp = millis();
    _state = STATE_WAITING_ACK;
    _rxBuffer = ""; // clear buffer for ack
}

void Rs485TouchScreen::displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount, int latestDiameter, int latestScanCount, int latestLengthLevel) {
    if (_state != STATE_IDLE) {
        return; 
    }

    StaticJsonDocument<512> doc;
    doc["page"] = "dashboard";
    JsonObject data = doc.createNestedObject("data");
    data["frame_counter"] = _frameCounter++; 
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
            Serial.println("[Rs485TouchScreen] RX Timeout (No response from slave)");
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
    
    if (!cleanLine.startsWith("$")) {
        if (cleanLine.length() > 0) {
            Serial.println("[Rs485TouchScreen] Skip: Not starting with $");
        }
        return;
    }
    
    int starIdx = cleanLine.lastIndexOf('*');
    if (starIdx < 1) {
        Serial.println("[Rs485TouchScreen] Skip: No * found or invalid position");
        return;
    }

    String jsonStr = cleanLine.substring(1, starIdx);
    String crcHexStr = cleanLine.substring(starIdx + 1);

    uint8_t calcCrc = calculateCRC8(jsonStr.c_str(), jsonStr.length());
    uint8_t recvCrc = (uint8_t) strtol(crcHexStr.c_str(), NULL, 16);

    if (calcCrc != recvCrc) {
        Serial.printf("[Rs485TouchScreen] CRC mismatch! Calc:%02X Recv:%02X, Raw Line: %s\n", calcCrc, recvCrc, cleanLine.c_str());
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    if (error) {
        Serial.print(F("[Rs485TouchScreen] deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    static String lastJsonStr = "";
    static uint32_t printCounter = 0;
    
    printCounter++;
    if (jsonStr != lastJsonStr || printCounter >= 20) {
        if (jsonStr != lastJsonStr) {
            Serial.println("\n[Rs485TouchScreen] 收到从机 JSON 数据 (内容有变化):");
        } else {
            Serial.println("\n[Rs485TouchScreen] 收到从机 JSON 数据 (心跳打印):");
        }
        serializeJsonPretty(doc, Serial);
        Serial.println();
        lastJsonStr = jsonStr;
        printCounter = 0;
    }


    // Capture slave page
    if (doc.containsKey("page")) {
        _slavePage = doc["page"].as<String>();
    }

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
            } else if (cmd == "set_outlet") {
                int index = ev["index"] | 0;
                float min = ev["min"] | 0.0f;
                float max = ev["max"] | 0.0f;
                uint8_t mask = ev["mask"] | 0;
                
                if (index < NUM_OUTLETS) {
                    sorter.setOutletMinDiameter(index, (int)(min + 0.05f)); 
                    sorter.setOutletMaxDiameter(index, (int)(max + 0.05f));
                    if (sorter.getOutlet(index)) {
                        sorter.getOutlet(index)->setTargetLength(mask);
                    }
                    sorter.saveConfig();
                    Serial.printf("[HMI] Applied Outlet #%d Config: Min=%.1f Max=%.1f Mask=%d\n", index+1, min, max, mask);
                }
            } else if (cmd == "diag_outlet") {
                int index = ev["index"] | 0;
                int targetState = ev["state"] | 0;
                if (index < NUM_OUTLETS) {
                    sorter.setOutletState(index, targetState == 1);
                    Serial.printf("[HMI] Diagnostic Command Outlet #%d -> %s\n", index + 1, (targetState == 1) ? "OPEN" : "CLOSE");
                }
            }
        }
    }
}

void Rs485TouchScreen::displayDiagnosticValues(const String& title, const String& value1, const String& value2) {
    if (_state != STATE_IDLE) return;
    
    // 如果从机在编码器页面，发送完整的编码器诊断 JSON
    if (_slavePage == "diag_encoder") {
        Encoder* enc = Encoder::getInstance();
        
        StaticJsonDocument<512> doc;
        doc["page"] = "diag_encoder";
        JsonObject data = doc.createNestedObject("data");
        
        data["raw_val"]       = enc->getRawCount();
        data["corrected_val"] = enc->getRawCount();
        data["logic_val"]     = enc->getCurrentPosition();
        data["zero_count"]    = enc->getZeroCrossCount();
        data["zero_total"]    = enc->getZeroCrossCount();
        data["zero_correct"]  = enc->getZeroCrossCount() - enc->getForcedZeroCount();
        
        data["pulse_count"]   = enc->getRawCount();
        data["velocity"]      = 0.0f;
        data["status"]        = 1;
        
        String jsonStr;
        serializeJson(doc, jsonStr);
        sendPayload(jsonStr);
    } else if (_slavePage == "diag_outlets" || _slavePage == "config_outlets") {
        StaticJsonDocument<1024> doc;
        doc["page"] = _slavePage; // Echo back the requested page
        JsonArray data = doc.createNestedArray("data");
        for (int i = 0; i < NUM_OUTLETS; i++) {
            JsonObject obj = data.createNestedObject();
            obj["min"] = (float)sorter.getOutletMinDiameter(i);
            obj["max"] = (float)sorter.getOutletMaxDiameter(i);
            obj["mask"] = (uint8_t)sorter.getOutlet(i)->getTargetLength();
            // 如果是诊断页，额外包含实时状态
            if (_slavePage == "diag_outlets") {
                obj["state"] = sorter.getOutlet(i)->isPositionOpen() ? 1 : 0;
            }
        }
        String jsonStr;
        serializeJson(doc, jsonStr);
        sendPayload(jsonStr);
    }
}

void Rs485TouchScreen::displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3, const String& line4, const String& line5) {
    // 复用 displayDiagnosticValues 的逻辑，确保多行模式下也能同步编码器数据
    displayDiagnosticValues(title, line1, line2);
}

void Rs485TouchScreen::displayScannerEncoderValues(const int* risingValues, const int* fallingValues) {
    if (_state != STATE_IDLE) return;
    
    // 如果从机在激光扫描仪页面，发送完整的激光波形数据
    if (_slavePage == "diag_laser") {
        DiameterScanner* ds = DiameterScanner::getInstance();
        StaticJsonDocument<1536> doc;
        doc["page"] = "diag_laser";
        JsonObject data = doc.createNestedObject("data");
        
        // 1. 当前电平状态位掩码 (Bit 0-NUM_SCAN_POINTS-1)
        uint8_t states = 0;
        for(int i=0; i<NUM_SCAN_POINTS; i++) {
            if (digitalRead(PINS_SCANNER[i]) == HIGH) {
                states |= (1 << i);
            }
        }
        data["states"] = states;
        
        // 2. 5路历史数据 Hex 字符串 (200 bits = 25 bytes per channel)
        const char* keys[] = {"history_p1", "history_p2", "history_p3", "history_p4"};
        char hexBuf[51]; 
        
        for(int i=0; i<NUM_SCAN_POINTS; i++) {
            memset(hexBuf, 0, sizeof(hexBuf));
            for(int j=0; j<25; j++) {
                uint8_t byteVal = 0;
                for(int bit=0; bit<8; bit++) {
                    int sampleIdx = j*8 + bit;
                    // 如果采样数还不够，默认为 0
                    if (sampleIdx < ds->getSampleCount()) {
                        if (ds->getSample(i, sampleIdx) == 1) {
                            byteVal |= (1 << (7 - bit));
                        }
                    }
                }
                sprintf(hexBuf + j*2, "%02X", byteVal);
            }
            data[keys[i]] = (const char*)hexBuf;
        }
        
        String jsonStr;
        serializeJson(doc, jsonStr);
        sendPayload(jsonStr);
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
