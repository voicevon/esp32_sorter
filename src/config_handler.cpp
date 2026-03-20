#include "config_handler.h"

// =========================
// DiameterConfigHandler实现
// =========================

void DiameterConfigHandler::initializeMode() {
  refreshDisplay();
}

void DiameterConfigHandler::refreshDisplay() {
  if (currentSubMode == 16) {
      userInterface->displayDiagnosticInfo("Config Diameter", 
        String("Action: SAVE & EXIT\n") +
        "Status: Ready\n" +
        "Click to return...");
      return;
  }
  
  int currentOutlet = currentSubMode / 2;
  bool editingMin = currentSubMode % 2 == 0;
  userInterface->displayDiagnosticInfo("Config Diameter", 
    "Outlet: " + String(currentOutlet) + "\n" +
    "Min: " + String(sorter->getOutletMinDiameter(currentOutlet)) + "\n" +
    "Max: " + String(sorter->getOutletMaxDiameter(currentOutlet)) + "\n" +
    "Editing: " + (editingMin ? "Min" : "Max"));
}

void DiameterConfigHandler::handleSubModeChange() {
  refreshDisplay();
}


void DiameterConfigHandler::handleValueChange(int delta) {
  if (currentSubMode == 16) return; // Exit mode doesn't change values
  
  int currentOutlet = currentSubMode / 2;
  bool editingMin = currentSubMode % 2 == 0;
  int currentValue = editingMin ? sorter->getOutletMinDiameter(currentOutlet) : sorter->getOutletMaxDiameter(currentOutlet);
  
  currentValue = constrain(currentValue + delta, 0, 255); // 使用constrain
  
  if (editingMin) {
    sorter->setOutletMinDiameter(currentOutlet, currentValue);
  } else {
    sorter->setOutletMaxDiameter(currentOutlet, currentValue);
  }
  
  refreshDisplay();
}

// =========================
// ServoConfigHandler 实现
// =========================

#include "modbus_controller.h"

void ServoConfigHandler::loadFromEEPROM() {
    uint8_t magic = EEPROM.read(EEPROM_BASE_ADDR);
    if (magic == EEPROM_MAGIC) {
        EEPROM.get(EEPROM_BASE_ADDR + 1, _accelMs);
        EEPROM.get(EEPROM_BASE_ADDR + 5, _decelMs);
        EEPROM.get(EEPROM_BASE_ADDR + 9, _fwdTorque);
        EEPROM.get(EEPROM_BASE_ADDR + 13, _revTorque);
        EEPROM.get(EEPROM_BASE_ADDR + 17, _maxSpeed);
        EEPROM.get(EEPROM_BASE_ADDR + 21, _safePowerUp);
    } else {
        saveToEEPROM(); 
    }
}

void ServoConfigHandler::saveToEEPROM() {
    EEPROM.write(EEPROM_BASE_ADDR, EEPROM_MAGIC);
    EEPROM.put(EEPROM_BASE_ADDR + 1, _accelMs);
    EEPROM.put(EEPROM_BASE_ADDR + 5, _decelMs);
    EEPROM.put(EEPROM_BASE_ADDR + 9, _fwdTorque);
    EEPROM.put(EEPROM_BASE_ADDR + 13, _revTorque);
    EEPROM.put(EEPROM_BASE_ADDR + 17, _maxSpeed);
    EEPROM.put(EEPROM_BASE_ADDR + 21, _safePowerUp);
    EEPROM.commit();
}

void ServoConfigHandler::initializeMode() {
    loadFromEEPROM();
    Serial.println("[SERVO-CONFIG] Mode Activated. SafeSON is " + String(_safePowerUp ? "ON" : "OFF"));
    refreshDisplay(); // 立即渲染 UI，消除黑屏或滞后感
}

void ServoConfigHandler::handleSubModeChange() {
    refreshDisplay();
}

void ServoConfigHandler::handleValueChange(int delta) {
    switch (currentSubMode) {
        case 0: _accelMs = constrain(_accelMs + delta * 10, 10, 5000); break;
        case 1: _decelMs = constrain(_decelMs + delta * 10, 10, 5000); break;
        case 2: _maxSpeed = constrain(_maxSpeed + delta * 50, 100, 3000); break;
        case 3: _fwdTorque = constrain(_fwdTorque + delta * 5, 0, 300); break;
        case 4: _revTorque = constrain(_revTorque + delta * 5, 0, 300); break;
        case 5: _safePowerUp = constrain(_safePowerUp + delta, 0, 1); break;
    }
    
    saveToEEPROM();
    applyCurrentParam(); 
    refreshDisplay();
}


void ServoConfigHandler::applyToServo() {
    auto m = ModbusController::getInstance();
    m->writeRegister(0x000B, _accelMs);   // PA11
    delay(50);
    m->writeRegister(0x000C, _decelMs);   // PA12
    delay(50);
    m->writeRegister(0x001E, _maxSpeed);  // PA30
    delay(50);
    m->writeRegister(0x0022, _fwdTorque); // PA34
    delay(50);
    m->writeRegister(0x0024, _revTorque); // PA36 
    delay(50);
    // [底层安全锁定] PA-53 位配置 
    // 0 = 外部/协议使能时机 (手动上电使能)
    // 1 = 内部强制由上电自动使能 (自动上电使能)
    m->writeRegister(0x0035, _safePowerUp == 1 ? 0 : 1); 
}

void ServoConfigHandler::applyCurrentParam() {
    auto m = ModbusController::getInstance();
    switch (currentSubMode) {
        case 0: m->writeRegister(0x008B, _accelMs); break; // PA11 + 0x80
        case 1: m->writeRegister(0x008C, _decelMs); break; // PA12 + 0x80
        case 2: m->writeRegister(0x009E, _maxSpeed); break; 
        case 3: 
            m->writeRegister(0x00A2, _fwdTorque); 
            m->writeRegister(0x0018, _fwdTorque); 
            break;
        case 4: 
            m->writeRegister(0x00A4, _revTorque); // PA36 + 0x80
            break;
        case 5:
            m->writeRegister(0x00B5, _safePowerUp == 1 ? 0 : 1); // PA53 + 0x80
            break;
    }
}

String ServoConfigHandler::subModeName() const {
    switch (currentSubMode) {
        case 0: return "Accel Time";
        case 1: return "Decel Time";
        case 2: return "Max Speed";
        case 3: return "Fwd Torque";
        case 4: return "Rev Torque";
        case 5: return "Safe Powerup";
        case 6: return "Save & Exit";
        default: return "Unknown";
    }
}

int ServoConfigHandler::currentValue() const {
    switch (currentSubMode) {
        case 0: return _accelMs;
        case 1: return _decelMs;
        case 2: return _maxSpeed;
        case 3: return _fwdTorque;
        case 4: return _revTorque;
        case 5: return _safePowerUp;
        case 6: return 0;
        default: return 0;
    }
}

void ServoConfigHandler::refreshDisplay() {
    if (currentSubMode == 6) {
        userInterface->displayDiagnosticInfo("Servo Config", 
            "Action:\nSAVE & EXIT\n\nClick to finish");
        return;
    }

    // 确定基础寄存器地址 (P0 组)
    uint16_t baseAddr = 0;
    switch (currentSubMode) {
        case 0: baseAddr = 0x000B; break;
        case 1: baseAddr = 0x000C; break;
        case 2: baseAddr = 0x001E; break;
        case 3: baseAddr = 0x0022; break;
        case 4: baseAddr = 0x0024; break;
        case 5: baseAddr = 0x0035; break;
    }

    // 执行回读 (由于 PS100 暂存区通常不可读，我们直接读取 P0 地址，它反映的是活跃值)
    auto m = ModbusController::getInstance();
    delay(10);
    uint16_t drvVal = m->readRegisterSync(baseAddr);

    String unit = " ms";
    if (currentSubMode == 2) unit = " RPM";
    else if (currentSubMode >= 3 && currentSubMode <= 4) unit = " %";
    else if (currentSubMode == 5) unit = ""; 

    String tgtStr = String(currentValue());
    if (currentSubMode == 5) tgtStr = _safePowerUp ? "ON" : "OFF";

    String drvStr = (drvVal == 0xFFFF) ? "COMM ERR" : String(drvVal);
    bool isMatched = false;

    if (currentSubMode == 5 && drvVal != 0xFFFF) {
        // PA53 特殊处理: _safePowerUp=1(ON) -> 写入0(Manual), _safePowerUp=0(OFF) -> 写入1(Auto)
        int expectedDrvBit = (_safePowerUp == 1) ? 0 : 1;
        drvStr = (drvVal == 1) ? "Auto" : "Manual";
        isMatched = (drvVal == expectedDrvBit);
    } else if (drvVal != 0xFFFF) {
        isMatched = (drvVal == (uint16_t)currentValue());
    }

    String status = isMatched ? "[Synced OK]" : "[Syncing...]";
    if (drvVal == 0xFFFF) status = "[No Response]";

    userInterface->displayDiagnosticInfo("Servo [" + subModeName() + "]", 
        "TARGET: " + tgtStr + unit + "\n" +
        "DRIVER: " + drvStr + unit + "\n" +
        status + "\n" + 
        " Knob to Adj / Click to Next");
}
