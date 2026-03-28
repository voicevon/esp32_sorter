#ifndef SHIFT_REGISTER_DRIVER_H
#define SHIFT_REGISTER_DRIVER_H

#include <Arduino.h>

/**
 * @brief 74HC595 移位寄存器硬件驱动
 * 负责 3 个级联芯片的串屏输出（芯片2：出口4-7，芯片1：出口0-3，芯片0：LED）
 */
class ShiftRegisterDriver {
private:
    uint32_t lastShiftData;
    int dsPin, shcpPin, stcpPin;

public:
    ShiftRegisterDriver(int ds, int shcp, int stcp);
    
    // 初始化引脚
    void initialize();
    
    // 写入数据（带脏检查）
    void write(uint8_t chip2, uint8_t chip1, uint8_t led);
    
    // 强制刷新（忽略脏检查）
    void forceUpdate(uint8_t chip2, uint8_t chip1, uint8_t led);
};

#endif // SHIFT_REGISTER_DRIVER_H
