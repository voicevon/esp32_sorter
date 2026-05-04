#include "hmi_factory.h"
#include "drv_oled_rotary/oled.h"
#include "drv_terminal/terminal.h"
#include "drv_mcgs/mcgs_display.h"
#include "drv_rs485_screen/rs485_touch_screen.h"

void HmiFactory::setupHmi(HmiType type) {
    UserInterface* ui = UserInterface::getInstance();
    
    // 1. 清除当前所有 HMI 绑定（实现动态切换能力）
    ui->clearAllDisplayDevices();
    ui->clearAllInputSources();
    
    // 2. 根据类型进行配对生产
    switch (type) {
        case HmiType::TERMINAL:
            // 终端模式：仅串口输出，无特定输入源（或可扩展 SerialInputSource）
            ui->addExternalDisplayDevice(Terminal::getInstance());
            ui->enableOutputChannel(OUTPUT_SERIAL);
            ui->disableOutputChannel(OUTPUT_OLED);
            Serial.println("[HmiFactory] Configured: TERMINAL Mode");
            break;
            
        case HmiType::OLED_ROTARY:
            // 经典模式：OLED 显示 + 物理编码器输入
            ui->addExternalDisplayDevice(OLED::getInstance());
            ui->addInputSource(RotaryInputSource::getInstance());
            ui->enableOutputChannel(OUTPUT_OLED);
            ui->disableOutputChannel(OUTPUT_SERIAL);
            Serial.println("[HmiFactory] Configured: OLED_ROTARY Mode");
            break;
            
        case HmiType::MCGS:
            // 现代模式：MCGS 串口屏 (自身兼具显示与输入)
            {
                // 使用静态局部变量或单例管理 MCGS 实例
                static McgsDisplay mcgs;
                mcgs.initialize();
                ui->addExternalDisplayDevice(&mcgs);
                ui->addInputSource(&mcgs);
                
                // MCGS 通讯属于私有协议，不占用通用的 OUTPUT_SERIAL 文本输出
                ui->disableOutputChannel(OUTPUT_SERIAL);
                ui->disableOutputChannel(OUTPUT_OLED);
                Serial.println("[HmiFactory] Configured: MCGS Mode");
            }
            break;
            
        case HmiType::LVGL_TOUCHSCREEN:
            // 自研 RS485 触控屏 (主从架构, 轮询拉取事件)
            ui->addExternalDisplayDevice(Rs485TouchScreen::getInstance());
            ui->addInputSource(Rs485TouchScreen::getInstance());
            ui->disableOutputChannel(OUTPUT_SERIAL); // 根据需要关闭
            ui->disableOutputChannel(OUTPUT_OLED);
            Serial.println("[HmiFactory] Configured: RS485_TOUCHSCREEN Mode");
            break;
            
        case HmiType::NONE:
        default:
            ui->disableOutputChannel(OUTPUT_ALL);
            Serial.println("[HmiFactory] Configured: NONE (Headless)");
            break;
    }
}
