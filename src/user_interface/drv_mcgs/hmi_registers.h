#ifndef HMI_REGISTERS_H
#define HMI_REGISTERS_H

#include <Arduino.h>

/**
 * @file hmi_registers.h
 * @brief MCGS TPC7062Ti (800×480) Modbus 寄存器地址映射
 *
 * 所有地址为 Modbus 协议中的"从 0 起的寄存器偏移"。
 * MCGS 组态软件中显示的 4xxxx 地址 = 本文件偏移 + 40001。
 * 
 * 完整的地址规划参见 mcgs_integration_plan.md §1。
 */

// ── §A  系统状态区 (ESP32 → Screen, Read-Only) ──────────────────────────────
constexpr uint16_t HMI_REG_SYS_MODE        = 0;    // 40001  uint16  0:Normal,1:DiagEncoder,...
constexpr uint16_t HMI_REG_LED_BITMAP       = 1;    // 40002  uint16  bit0=出口0, bit7=出口7
constexpr uint16_t HMI_REG_CONVEYOR_LOAD    = 2;    // 40003  uint16  0–1000 (0.0–100.0%)
constexpr uint16_t HMI_REG_FIRMWARE_VER     = 3;    // 40004  uint16  BCD 码, e.g. 0x0200=v2.0
constexpr uint16_t HMI_REG_HEARTBEAT        = 4;    // 40005  uint16  心跳值, 每秒+1

// ── §B  生产数据区 (ESP32 → Screen, Read-Only) ──────────────────────────────
constexpr uint16_t HMI_REG_DIAMETER         = 9;    // 40010  uint16  ×100, e.g.2250=22.50mm
constexpr uint16_t HMI_REG_SPEED_PER_HOUR   = 10;   // 40011  uint16  pcs/h
constexpr uint16_t HMI_REG_TOTAL_HI         = 11;   // 40012  uint16  总计数高位
constexpr uint16_t HMI_REG_TOTAL_LO         = 12;   // 40013  uint16  总计数低位
constexpr uint16_t HMI_REG_IDENT_HI         = 13;   // 40014  uint16  识别计数高位
constexpr uint16_t HMI_REG_IDENT_LO         = 14;   // 40015  uint16  识别计数低位
constexpr uint16_t HMI_REG_DIAG_VAL1        = 15;   // 40016  uint16  诊断辅助值1 (模式相关)
constexpr uint16_t HMI_REG_DIAG_VAL2        = 16;   // 40017  uint16  诊断辅助值2 (模式相关)

// ── §C  配置参数区 (ESP32 → Screen, 上电同步) ───────────────────────────────
// 每个出口 3 个寄存器: [CFG_BASE + outlet*3 + 0] = Min
//                      [CFG_BASE + outlet*3 + 1] = Max
//                      [CFG_BASE + outlet*3 + 2] = LenMask
// 共 8 出口 × 3 = 24 寄存器，地址 40020–40043
constexpr uint16_t HMI_REG_CFG_BASE         = 19;   // 40020  首地址

// 辅助宏：按出口索引计算具体寄存器偏移
#define HMI_REG_OUTLET_MIN(n)     (HMI_REG_CFG_BASE + (n) * 3 + 0)
#define HMI_REG_OUTLET_MAX(n)     (HMI_REG_CFG_BASE + (n) * 3 + 1)
#define HMI_REG_OUTLET_LENMASK(n) (HMI_REG_CFG_BASE + (n) * 3 + 2)

// ── §D  出口工作模式区 (ESP32 → Screen, 上电同步) ────────────────────────────
// 每个出口 1 个寄存器: 0=多物体模式, 1=直径模式
// 地址 40044–40051 (8 出口)
constexpr uint16_t HMI_REG_OUTLET_MODE_BASE = 43;   // 40044  首地址

#define HMI_REG_OUTLET_MODE(n)    (HMI_REG_OUTLET_MODE_BASE + (n))

// ── §E  触控控制区 (Screen → ESP32, Read-Write) ──────────────────────────────
constexpr uint16_t HMI_REG_CMD_CODE         = 99;   // 40100  uint16  触控命令(握手); ESP32回写0
constexpr uint16_t HMI_REG_PARAM_IDX        = 100;  // 40101  uint16  参数修改目标出口 (0–7)
constexpr uint16_t HMI_REG_PARAM_FIELD      = 101;  // 40102  uint16  0:Min,1:Max,2:LenMask,3:Mode
constexpr uint16_t HMI_REG_PARAM_VALUE      = 102;  // 40103  uint16  新参数值 (Min/Max 已×100)
constexpr uint16_t HMI_REG_CMD_CONFIRM      = 103;  // 40104  uint16  1:WRITE_EEPROM; ESP32回写0

// ── 触控命令码枚举 (CmdCode 值) ──────────────────────────────────────────────
namespace HmiCmd {
    constexpr uint16_t NONE  = 0;
    constexpr uint16_t MENU  = 1;  // 进入菜单 (等同长按旋钮)
    constexpr uint16_t OK    = 2;  // 确认 (等同短按旋钮)
    constexpr uint16_t UP    = 3;  // 上翻 (等同编码器+)
    constexpr uint16_t DOWN  = 4;  // 下翻 (等同编码器-)
    constexpr uint16_t BACK  = 5;  // 返回上级
}

// ── 参数字段枚举 (ParamField 值) ─────────────────────────────────────────────
namespace HmiField {
    constexpr uint16_t MIN      = 0;
    constexpr uint16_t MAX      = 1;
    constexpr uint16_t LEN_MASK = 2;
    constexpr uint16_t MODE     = 3;
}

#endif // HMI_REGISTERS_H
