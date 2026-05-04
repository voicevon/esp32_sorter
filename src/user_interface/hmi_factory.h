#ifndef HMI_FACTORY_H
#define HMI_FACTORY_H

#include "user_interface.h"

/**
 * @enum HmiType
 * @brief HMI 交互组合类型
 */
enum class HmiType {
    NONE,           // 无 HMI
    TERMINAL,       // 串口终端 (仅输出)
    OLED_ROTARY,    // OLED + 物理编码器 (传统配对)
    MCGS,           // MCGS 串口屏 (一体化)
    LVGL_TOUCHSCREEN// 新增: 自研 RS485 JSON 触控屏
};

/**
 * @class HmiFactory
 * @brief 统一 HMI 工厂类
 * 
 * 负责根据 HmiType 配对实例化 Display 和 InputSource，
 * 并注入到 UserInterface 管理器中。
 */
class HmiFactory {
public:
    /**
     * @brief 一键配置 HMI 环境
     * @param type 目标 HMI 类型
     */
    static void setupHmi(HmiType type);
};

#endif // HMI_FACTORY_H
