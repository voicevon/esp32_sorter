#pragma once

#include "esp32servo.h"
#include "modular/pins.h"
#include "main.h"

/**
 * 上料器测试模式处理类
 * 负责处理上料器测试模式下的所有功能
 */
class ReloaderTestHandler {
private:
    // 上料器舵机
    Servo reloaderServo;
    
    // 上料器舵机角度定义
    static const int RELOADER_OPEN_ANGLE = 90;
    static const int RELOADER_CLOSE_ANGLE = 180;
    
public:
    /**
     * 构造函数
     */
    ReloaderTestHandler();
    
    /**
     * 初始化上料器测试模式
     */
    void initialize();
    
    /**
     * 开启上料器
     */
    void openReloader();
    
    /**
     * 关闭上料器
     */
    void closeReloader();
    
    /**
     * 处理上料器相关任务
     */
    void processTasks();
};
