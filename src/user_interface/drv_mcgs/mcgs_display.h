#ifndef MCGS_DISPLAY_H
#define MCGS_DISPLAY_H

#include <Arduino.h>
#include <EEPROM.h>
#include "../common/display.h"
#include "../common/input_source.h"
#include "../../utils/ModbusMaster.h"
#include "hmi_registers.h"
#include "../../config.h"

/**
 * @class McgsDisplay
 * @brief 昆仑通态 TPC7062Ti (800×480) 串口屏 Modbus RTU 显示驱动
 *
 * 继承 Display 抽象基类，作为第三个显示设备注入 UserInterface。
 *
 * 架构说明：
 *  - 屏幕为 Modbus RTU 从机，ESP32 为主机
 *  - 数据推送方向：ESP32 → 屏幕（仪表盘、状态、配置参数）
 *  - 指令接收方向：屏幕 → ESP32（触控命令通过 CmdCode 握手）
 *
 * 调用方式（main.cpp）：
 *  setup():
 *    mcgsDisplay.initialize();
 *    UserInterface::addExternalDisplayDevice(&mcgsDisplay);
 *
 *  vUITask() 主循环（30ms 节拍）：
 *    mcgsDisplay.pollCommand();
 *
 *  vControlTask() 每 500ms：
 *    mcgsDisplay.pushProductionData(...);
 */
class McgsDisplay : public Display, public InputSource {
public:
    McgsDisplay();
    ~McgsDisplay() override = default;

    // ── 设备生命周期 ──────────────────────────────────────────────────────────
    /**
     * @brief 初始化 UART2 + RS-485，启动 Modbus 任务，上电同步 EEPROM 配置
     * 必须在 setup() 中调用，在 EEPROM.begin() 之后。
     */
    void initialize() override;

    /** 返回 Modbus 通讯是否已就绪 */
    bool isAvailable() const override { return _isAvailable; }

    // ── HMI 专用接口（在 main.cpp 各任务中调用） ─────────────────────────────
    /**
     * @brief 轮询屏幕触控命令（在 vUITask 30ms 节拍中调用）
     * 读取 HMI_REG_CMD_CODE，若非 0 则处理并回写 0（握手完成）。
     */
    void pollCommand();

    /**
     * @brief 推送生产数据到仪表盘寄存器（在 vControlTask 每 500ms 调用）
     * @param diameter     实时直径 (×100 整数，例如 2250 = 22.50mm)
     * @param speedPerHour 分拣速度 (pcs/h)
     * @param totalCount   总计数 (uint32)
     * @param identCount   识别计数 (uint32)
     */
    void pushProductionData(uint16_t diameter, uint16_t speedPerHour,
                            uint32_t totalCount, uint32_t identCount);

    /**
     * @brief 将 EEPROM 中的出口配置同步到屏幕寄存器（initialize() 内部调用）
     * 写入 40020–40051 区段（Min / Max / LenMask / Mode × 8 出口）。
     */
    void syncConfigToScreen();

    // ── Display 接口实现 ──────────────────────────────────────────────────────
    // ── InputSource 接口实现 ──────────────────────────────────────────────
    void tick() override { pollCommand(); }
    bool hasIntent() override;
    UIIntent pollIntent() override;
    String getName() const override { return "McgsDisplay"; }

    // 以下方法由 UserInterface 统一调度，McgsDisplay 实现有意义的子集，
    void renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) override {}

    // 核心快照刷新接口
    void refresh(const DisplaySnapshot& snapshot) override;

    // 清理屏幕
    void clearDisplay() override {}

private:
    ModbusMaster _modbus;
    bool         _isAvailable = false;
    uint8_t      _slaveId;

    // LedBitmap 本地缓存（按 bit 更新，批量写入）
    uint16_t _ledBitmap = 0;

    // 触控命令读取缓冲区
    uint16_t _cmdRegs[5] = {0}; // CmdCode, ParamIdx, ParamField, ParamValue, CmdConfirm

    // ── 内部辅助 ─────────────────────────────────────────────────────────────
    /** 处理触控命令并注入对应 KeyEvent 到 UserInterface */
    void handleCmdCode(uint16_t cmd);

    /** 处理配置写入请求（ParamIdx/Field/Value → EEPROM）*/
    void handleParamWrite(uint8_t outletIdx, uint8_t field, uint16_t value);

    /** 处理 EEPROM 持久化确认命令 */
    void handleCmdConfirm();

    // ── 输入意图 ─────────────────────────────────────────────────────────────
    UIIntent _pendingIntent;

    // ── 心跳测试 ─────────────────────────────────────────────────────────────
    uint16_t _heartbeat = 0;
    uint32_t _lastHeartbeatUpdate = 0;
};

#endif // MCGS_DISPLAY_H
