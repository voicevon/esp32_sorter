#include "rs485_touch_screen.h"
#include "../../config.h"
#include "../../modular/encoder.h"
#include "../../modular/diameter_scanner.h"
#include "../../modular/sorter.h"
#include <EEPROM.h>

extern Sorter sorter;

Rs485TouchScreen* Rs485TouchScreen::getInstance() {
    static Rs485TouchScreen instance;
    return &instance;
}

Rs485TouchScreen::Rs485TouchScreen() : _serial(&Serial2), _state(STATE_IDLE), _txTimestamp(0), _slavePage("dashboard") {}

void Rs485TouchScreen::initialize() {
    Serial.println("[Rs485TouchScreen] >> Initializing RS485 HMI Touch Screen driver...");
    Serial.printf("[Rs485TouchScreen] Pin Config -> RX Pin: %d, TX Pin: %d, EN Pin: %d, Baudrate: %d\n", 
                  PIN_HMI_485_RX, PIN_HMI_485_TX, PIN_HMI_485_EN, HMI_MODBUS_BAUD);
    
    pinMode(PIN_HMI_485_EN, OUTPUT);
    digitalWrite(PIN_HMI_485_EN, LOW); // Set to RX mode by default
    _serial->begin(HMI_MODBUS_BAUD, SERIAL_8N2, PIN_HMI_485_RX, PIN_HMI_485_TX);
    _rxBuffer.reserve(512);
    
    Serial.println("[Rs485TouchScreen] Serial2 initialized as SERIAL_8N2. Driver ready.");
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

    // Log the outgoing payload for debugging
    // Serial.printf("[Rs485TouchScreen] >> TX RAW: %s", outStr.c_str());
}

void Rs485TouchScreen::refresh(const DisplaySnapshot& snapshot) {
    String jsonStr;
    bool hasData = false;

    if (_slavePage == "dashboard" && snapshot.currentMode == APP_PRODUCTION) {
        StaticJsonDocument<512> doc;
        doc["page"] = "dashboard";
        JsonObject data = doc.createNestedObject("data");
        data["frame_counter"] = _frameCounter++; 
        data["speed"] = snapshot.data.dashboard.sortingSpeedPerSecond;
        data["yield"] = snapshot.data.dashboard.identifiedCount;
        data["capacity"] = snapshot.data.dashboard.sortingSpeedPerHour;
        data["diameter"] = snapshot.data.dashboard.latestDiameter;

        serializeJson(doc, jsonStr);
        hasData = true;
    } 
    else if (_slavePage == "diag_encoder" && (snapshot.currentMode == APP_DIAG_ENCODER || snapshot.currentMode == APP_DIAG_HMI || snapshot.currentMode == APP_CONFIG_PHASE_OFFSET)) {
        StaticJsonDocument<512> doc;
        doc["page"] = "diag_encoder";
        JsonObject data = doc.createNestedObject("data");
        
        data["raw_val"]       = snapshot.data.encoder.raw;
        data["corrected_val"] = snapshot.data.encoder.corrected;
        data["logic_val"]     = snapshot.data.encoder.logic;
        data["offset"]        = snapshot.data.encoder.offset;
        data["zero_count"]    = snapshot.data.encoder.zeroCount;
        data["zero_total"]    = snapshot.data.encoder.zeroTotal;
        data["zero_correct"]  = snapshot.data.encoder.zeroCorrect;
        
        data["pulse_count"]   = snapshot.data.encoder.raw;
        data["velocity"]      = (float)abs(snapshot.data.encoder.raw - _lastRawCount) * 10.0f;
        data["status"]        = 1;
        
        _lastRawCount = snapshot.data.encoder.raw;
        
        serializeJson(doc, jsonStr);
        hasData = true;
    } 
    else if ((_slavePage == "diag_outlets" || _slavePage == "config_outlets") && (snapshot.currentMode == APP_DIAG_OUTLET || snapshot.currentMode == APP_CONFIG_DIAMETER)) {
        StaticJsonDocument<1024> doc;
        doc["page"] = _slavePage;
        JsonArray data = doc.createNestedArray("data");
        for (int i = 0; i < NUM_OUTLETS; i++) {
            JsonObject obj = data.createNestedObject();
            obj["min"] = (float)snapshot.data.outlet.outlets[i].min;
            obj["max"] = (float)snapshot.data.outlet.outlets[i].max;
            obj["mask"] = (uint8_t)snapshot.data.outlet.outlets[i].mask;
            if (_slavePage == "diag_outlets") {
                obj["state"] = snapshot.data.outlet.outlets[i].isOpen ? 1 : 0;
            }
        }
        serializeJson(doc, jsonStr);
        hasData = true;
    } 
    else if (_slavePage == "diag_laser" && snapshot.currentMode == APP_DIAG_SCANNER) {
        StaticJsonDocument<1536> doc;
        doc["page"] = "diag_laser";
        JsonObject data = doc.createNestedObject("data");
        
        data["states"] = snapshot.data.scanner.states;
        
        const char* keys[] = {"history_p1", "history_p2", "history_p3", "history_p4"};
        int samples = snapshot.data.scanner.sampleCount;
        if (samples > 200) samples = 200;
        int numBytes = (samples + 7) / 8;
        
        char* hexBuf = (char*)malloc(numBytes * 2 + 1); 
        
        for(int i=0; i<NUM_SCAN_POINTS; i++) {
            memset(hexBuf, 0, numBytes * 2 + 1);
            for(int j=0; j<numBytes; j++) {
                uint8_t byteVal = snapshot.data.scanner.history[i][j];
                sprintf(hexBuf + j*2, "%02X", byteVal);
            }
            data[keys[i]] = (const char*)hexBuf;
        }
        free(hexBuf);
        
        serializeJson(doc, jsonStr);
        hasData = true;
    }

    if (hasData) {
        while (_txQueue.size() >= 5) {
            _txQueue.pop(); // Drop oldest to avoid memory leaks
        }
        _txQueue.push(jsonStr);
        
        static uint32_t lastPushLogMs = 0;
        if (millis() - lastPushLogMs >= 2000) {
            lastPushLogMs = millis();
            Serial.printf("[Rs485TouchScreen] Snapshot pushed to TX queue (Page: %s, Queue Size: %d)\n", _slavePage.c_str(), _txQueue.size());
        }
    }
}

void Rs485TouchScreen::tick() {
    // 1. 始终轮询串口接收缓存，防止在 STATE_IDLE 时丢包或遗漏触摸屏的主动上报事件
    while (_serial->available()) {
        char c = _serial->read();
        if (c == '\n') {
            // Serial.printf("[Rs485TouchScreen] << RX Complete Line (length: %d), processing...\n", _rxBuffer.length());
            processLine(_rxBuffer);
            _rxBuffer = "";
            _state = STATE_IDLE; // 收到完整帧后重置为空闲，允许发送下一帧
        } else {
            _rxBuffer += c;
            if (_rxBuffer.length() > 1024) {
                Serial.println("[Rs485TouchScreen] WARNING: RX Buffer overflow (>1024), clearing!");
                _rxBuffer = "";
            }
        }
    }

    // 2. 发送超时检测（仅在等待 ACK 状态下有效）
    if (_state == STATE_WAITING_ACK) {
        if (millis() - _txTimestamp > 300) { // 300ms 超时
            Serial.printf("[Rs485TouchScreen] RX Timeout! No response from slave on page [%s] for >300ms\n", _slavePage.c_str());
            _state = STATE_IDLE;
            _rxBuffer = "";
        }
    }

    // 3. 驱动队列发送
    if (_state == STATE_IDLE && !_txQueue.empty()) {
        String nextPayload = _txQueue.front();
        _txQueue.pop();
        
        static uint32_t lastTxLogMs = 0;
        if (millis() - lastTxLogMs >= 2000) {
            lastTxLogMs = millis();
            Serial.printf("[Rs485TouchScreen] Driving queue send (Queue remaining: %d, State: STATE_IDLE)\n", _txQueue.size());
        }
        
        sendPayload(nextPayload);
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

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    if (error) {
        Serial.printf("[Rs485TouchScreen] deserializeJson() failed: %s, Raw JsonStr: %s\n", error.c_str(), jsonStr.c_str());
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


    // Capture slave page and sync mode
    if (doc.containsKey("page")) {
        String newPage = doc["page"].as<String>();
        if (newPage != _slavePage) {
            _slavePage = newPage;
            
            // Map page string to SystemMode enum value
            int targetMode = -1;
            if (newPage == "dashboard") targetMode = 0; // APP_PRODUCTION
            else if (newPage == "diag_encoder") targetMode = 1; // APP_DIAG_ENCODER
            else if (newPage == "diag_laser") targetMode = 2; // APP_DIAG_SCANNER
            else if (newPage == "diag_outlets") targetMode = 3; // APP_DIAG_OUTLET
            else if (newPage == "config_outlets") targetMode = 4; // APP_CONFIG_DIAMETER
            else if (newPage == "about") targetMode = 5; // APP_VERSION_INFO
            
            if (targetMode != -1) {
                _intentQueue.push(UIIntent(UIAction::NAVIGATE_PATH, targetMode));
                Serial.printf("[RS485] Remote Page Sync -> Mode %d (%s)\n", targetMode, newPage.c_str());
            }
        }
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
            } else if (cmd == "set_offset") {
                int newOffset = ev["state"] | ev["params"] | 0; // Accept both for compatibility
                Encoder::getInstance()->setPhaseOffset(newOffset);
                
                // 持久化到 EEPROM
                EEPROM.write(EEPROM_ADDR_PHASE_OFFSET, 0xA5); // Magic byte
                EEPROM.write(EEPROM_ADDR_PHASE_OFFSET + 1, (uint8_t)newOffset);
                EEPROM.commit();
                
                Serial.printf("[HMI] Remote Offset Calibration -> %d (Saved to EEPROM)\n", newOffset);
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
