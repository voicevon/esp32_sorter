/**
 * @file mcgs_display.cpp
 * @brief 昆仑通态 TPC7062Ti (800×480) 串口屏 Modbus RTU 驱动实现
 *
 * EEPROM 布局（来自 config.h / sorter.cpp）：
 *   EEPROM_ADDR_DIAMETER      0x00  magic (0xAA = 已初始化)
 *   EEPROM_ADDR_DIAMETER_DATA 0x01  NUM_OUTLETS × 3 字节: [min, max, lenMask]
 *                                   每字节为整数单位 (mm × 1，非 mm × 100)
 *   EEPROM_ADDR_OUTLET0_MODE  0x19  出口 0 工作模式 (0=多物体, 1=直径)
 *                                   注：当前固件只持久化出口 0 的模式
 *
 * Modbus 寄存器规约详见 hmi_registers.h。
 */
#include "mcgs_display.h"
#include "../main.h"
#include "user_interface.h"

// ── 构造函数 ──────────────────────────────────────────────────────────────────
McgsDisplay::McgsDisplay()
    : _modbus(PIN_HMI_485_RX, PIN_HMI_485_TX, PIN_HMI_485_EN, HMI_MODBUS_BAUD),
      _slaveId(HMI_MODBUS_SLAVE_ID) {
}

// ── 初始化 ────────────────────────────────────────────────────────────────────
void McgsDisplay::initialize() {
    _modbus.begin();
    _isAvailable = true;

    Serial.println("[McgsDisplay] initialize() — Modbus RTU ready");

    // 上电将固件版本写入状态区 (40004)
    // FIRMWARE_VERSION 格式为 "2.0.0"；BCD 编码为高字节主版本、低字节次版本
    uint16_t fwVer = 0x0200; // 固定 v2.0，后续可解析 FIRMWARE_VERSION 字符串
    _modbus.syncWrite(_slaveId, HMI_REG_FIRMWARE_VER, fwVer);

    // 写 SysMode = 0 (Normal)，使 P0 的连接指示灯变绿、触发自动跳转 P1
    _modbus.syncWrite(_slaveId, HMI_REG_SYS_MODE, 0);

    // 同步 EEPROM 配置参数到屏幕
    syncConfigToScreen();
}

// ── 上电配置同步 ──────────────────────────────────────────────────────────────
void McgsDisplay::syncConfigToScreen() {
    // 检查 EEPROM magic
    if (EEPROM.read(EEPROM_ADDR_DIAMETER) != 0xAA) {
        Serial.println("[McgsDisplay] EEPROM not initialized, skipping config sync");
        return;
    }

    Serial.println("[McgsDisplay] Syncing EEPROM config to screen...");

    // 读出口 0 的工作模式（固件当前只持久化出口 0）
    uint8_t outlet0Mode = EEPROM.read(EEPROM_ADDR_OUTLET0_MODE);

    for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
        // EEPROM 存储的是 mm 整数（例如 15 = 15mm）
        uint8_t minVal  = EEPROM.read(EEPROM_ADDR_DIAMETER_DATA + i * 3);
        uint8_t maxVal  = EEPROM.read(EEPROM_ADDR_DIAMETER_DATA + i * 3 + 1);
        uint8_t lenMask = EEPROM.read(EEPROM_ADDR_DIAMETER_DATA + i * 3 + 2);

        // Modbus 寄存器使用 ×100 单位（例如 1500 = 15.00mm）
        _modbus.syncWrite(_slaveId, HMI_REG_OUTLET_MIN(i),     (uint16_t)minVal  * 100);
        _modbus.syncWrite(_slaveId, HMI_REG_OUTLET_MAX(i),     (uint16_t)maxVal  * 100);
        _modbus.syncWrite(_slaveId, HMI_REG_OUTLET_LENMASK(i), lenMask);

        // 工作模式：仅出口 0 有持久化值，其余默认 0（多物体）
        uint16_t mode = (i == 0) ? outlet0Mode : 0;
        _modbus.syncWrite(_slaveId, HMI_REG_OUTLET_MODE(i), mode);

        Serial.printf("[McgsDisplay]   Outlet%d: min=%d max=%d len=0x%02X mode=%d\n",
                      i, minVal, maxVal, lenMask, mode);
    }
    Serial.println("[McgsDisplay] Config sync done.");
}

// ── 触控命令轮询（在 vUITask 30ms 节拍中调用） ───────────────────────────────
void McgsDisplay::pollCommand() {
    if (!_isAvailable) return;

    // ── 心跳发送 (每 1000ms 加一) ──
    uint32_t now = millis();
    if (now - _lastHeartbeatUpdate >= 1000) {
        _lastHeartbeatUpdate = now;
        _heartbeat++;
        _modbus.asyncWrite(_slaveId, HMI_REG_HEARTBEAT, _heartbeat, nullptr);
    }

    // 异步读取 5 个触控控制寄存器（40100–40104）
    _modbus.asyncRead(_slaveId, HMI_REG_CMD_CODE, 5, 
        [this](Modbus::ResultCode result, uint16_t, void*) -> bool {
            if (result != Modbus::EX_SUCCESS) return false;

            uint16_t cmd     = _cmdRegs[0]; // CmdCode
            uint16_t idx     = _cmdRegs[1]; // ParamIdx
            uint16_t field   = _cmdRegs[2]; // ParamField
            uint16_t value   = _cmdRegs[3]; // ParamValue
            uint16_t confirm = _cmdRegs[4]; // CmdConfirm

            // 处理触控命令
            if (cmd != HmiCmd::NONE) {
                handleCmdCode(cmd);
                // 握手：立即回写 CmdCode = 0
                _modbus.asyncWrite(_slaveId, HMI_REG_CMD_CODE, 0, nullptr);
            }

            // 处理参数写入（OK 命令触发时，ParamField + ParamValue 有效）
            if (cmd == HmiCmd::OK && idx < NUM_OUTLETS) {
                handleParamWrite((uint8_t)idx, (uint8_t)field, value);
            }

            // 处理 EEPROM 持久化确认
            if (confirm != 0) {
                handleCmdConfirm();
                _modbus.asyncWrite(_slaveId, HMI_REG_CMD_CONFIRM, 0, nullptr);
            }

            return true;
        },
        _cmdRegs);
}

// ── 生产数据推送（在 vControlTask 每 500ms 调用）────────────────────────────
void McgsDisplay::pushProductionData(uint16_t diameter, uint16_t speedPerHour,
                                     uint32_t totalCount, uint32_t identCount) {
    if (!_isAvailable) return;

    // §B 生产数据区，8 个寄存器一次批量写入（但 asyncWrite 是单寄存器逐个写）
    // 实际上 McgsDisplay 自己调用 4 次 asyncWrite，间隔由 Modbus 任务保证时序
    _modbus.asyncWrite(_slaveId, HMI_REG_DIAMETER,  diameter,     nullptr);
    _modbus.asyncWrite(_slaveId, HMI_REG_SPEED_PER_HOUR, speedPerHour, nullptr);
    _modbus.asyncWrite(_slaveId, HMI_REG_TOTAL_HI,  (uint16_t)(totalCount >> 16),  nullptr);
    _modbus.asyncWrite(_slaveId, HMI_REG_TOTAL_LO,  (uint16_t)(totalCount & 0xFFFF), nullptr);
    _modbus.asyncWrite(_slaveId, HMI_REG_IDENT_HI,  (uint16_t)(identCount >> 16),  nullptr);
    _modbus.asyncWrite(_slaveId, HMI_REG_IDENT_LO,  (uint16_t)(identCount & 0xFFFF), nullptr);
}

// ── Display 接口：有意义的实现 ───────────────────────────────────────────────
void McgsDisplay::displayModeChange(SystemMode newMode) {
    if (!_isAvailable) return;
    _modbus.asyncWrite(_slaveId, HMI_REG_SYS_MODE, (uint16_t)newMode, nullptr);
}

void McgsDisplay::displayModeChange(const String& newModeName) {
    // 字符串版本：尝试映射到 SysMode 数值，未知模式写 0
    (void)newModeName;
    // 当前不做字符串解析，以 displayModeChange(SystemMode) 为主
}

void McgsDisplay::displayOutletStatus(uint8_t outletIndex, bool isOpen) {
    if (!_isAvailable || outletIndex >= 16) return;
    if (isOpen) {
        _ledBitmap |=  (1u << outletIndex);
    } else {
        _ledBitmap &= ~(1u << outletIndex);
    }
    _modbus.asyncWrite(_slaveId, HMI_REG_LED_BITMAP, _ledBitmap, nullptr);
}

void McgsDisplay::displayDiagnosticInfo(const String& title, const String& info) {
    if (!_isAvailable) return;
    // title → 从中提取模式值（暂以 atoi 粗解析），info 写 DiagValue1
    uint16_t diagVal = (uint16_t)info.toInt();
    _modbus.asyncWrite(_slaveId, HMI_REG_DIAG_VAL1, diagVal, nullptr);
}

void McgsDisplay::displayDashboard(float /*sortingSpeedPerSecond*/,
                                   int  /*sortingSpeedPerMinute*/,
                                   int    sortingSpeedPerHour,
                                   int    identifiedCount,
                                   int  /*transportedTrayCount*/,
                                   int    latestDiameter,
                                   int  /*latestScanCount*/,
                                   int  /*latestLengthLevel*/) {
    // 委托给 pushProductionData
    pushProductionData(
        (uint16_t)latestDiameter,       // 已是 ×100 单位
        (uint16_t)sortingSpeedPerHour,
        0,                               // totalCount 暂不从此接口传
        (uint32_t)identifiedCount
    );
}

void McgsDisplay::displayDiameter(int latestDiameter) {
    if (!_isAvailable) return;
    _modbus.asyncWrite(_slaveId, HMI_REG_DIAMETER, (uint16_t)latestDiameter, nullptr);
}

// ── 内部辅助 ──────────────────────────────────────────────────────────────────
void McgsDisplay::handleCmdCode(uint16_t cmd) {
    // 将 MCGS 触控命令转换为 UserInterface 按键事件注入
    // TODO: 待 UserInterface::injectKeyEvent() 实现后完善此处
    // 当前版本：仅打印日志，预留接口
    Serial.printf("[McgsDisplay] CmdCode=%d received\n", cmd);

    /*
     * 预期实现（UserInterface 扩展后启用）：
     *
     * switch (cmd) {
     *     case HmiCmd::MENU:  ui->injectKeyEvent(KEY_BTN_LONG);  break;
     *     case HmiCmd::OK:    ui->injectKeyEvent(KEY_BTN_CLICK); break;
     *     case HmiCmd::UP:    ui->injectKeyEvent(KEY_ENC_UP);    break;
     *     case HmiCmd::DOWN:  ui->injectKeyEvent(KEY_ENC_DOWN);  break;
     *     case HmiCmd::BACK:  ui->injectKeyEvent(KEY_BTN_CLICK); break;
     * }
     */
}

void McgsDisplay::handleParamWrite(uint8_t outletIdx, uint8_t field, uint16_t value) {
    // 屏幕传来的 value 对于 Min/Max 是 ×100 单位，需除以 100 存回 EEPROM
    if (outletIdx >= NUM_OUTLETS) return;

    int eepromOffset = EEPROM_ADDR_DIAMETER_DATA + outletIdx * 3;

    switch (field) {
        case HmiField::MIN:
            EEPROM.write(eepromOffset, (uint8_t)(value / 100));
            Serial.printf("[McgsDisplay] Write Outlet%d.Min = %d mm\n", outletIdx, value / 100);
            break;
        case HmiField::MAX:
            EEPROM.write(eepromOffset + 1, (uint8_t)(value / 100));
            Serial.printf("[McgsDisplay] Write Outlet%d.Max = %d mm\n", outletIdx, value / 100);
            break;
        case HmiField::LEN_MASK:
            EEPROM.write(eepromOffset + 2, (uint8_t)(value & 0x07));
            Serial.printf("[McgsDisplay] Write Outlet%d.LenMask = 0x%02X\n", outletIdx, value & 0x07);
            break;
        case HmiField::MODE:
            if (outletIdx == 0) {
                EEPROM.write(EEPROM_ADDR_OUTLET0_MODE, (uint8_t)(value & 0x01));
                Serial.printf("[McgsDisplay] Write Outlet0.Mode = %d\n", value & 0x01);
            }
            break;
        default:
            break;
    }
    // 注意：不在此处 commit()，等待 CmdConfirm=1 时统一持久化
}

void McgsDisplay::handleCmdConfirm() {
    EEPROM.commit();
    Serial.println("[McgsDisplay] EEPROM committed (triggered by CmdConfirm=1)");
}
