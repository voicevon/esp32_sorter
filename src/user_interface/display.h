#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "../main.h"  // 相对路径，因为这个文件将放在user_interface子目录中

// 系统工作模式前向声明
enum SystemMode;

/**
 * @class Display
 * @brief 显示设备抽象基类
 * 
 * 该类定义了所有显示设备必须实现的接口方法
 * OLED和Terminal类将继承自这个抽象基类
 */
class Display {
public:
    // 虚析构函数，确保子类能够正确析构
    virtual ~Display() = default;
    
    // 初始化显示设备
    virtual void initialize() = 0;
    
    // 检查显示设备是否可用
    virtual bool isAvailable() const = 0;
    
    // 显示模式变化信息（使用SystemMode枚举）
    virtual void displayModeChange(SystemMode newMode) = 0;
    
    // 显示模式变化信息（使用字符串）
    virtual void displayModeChange(const String& newModeName) = 0;
    
    // 显示出口状态变化
    virtual void displayOutletStatus(uint8_t outletIndex, bool isOpen) = 0;
    
    // 显示诊断信息
    virtual void displayDiagnosticInfo(const String& title, const String& info) = 0;
    
    // 显示出口测试模式图形
    virtual void displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) = 0;
    
    // 显示扫描仪编码器值
    virtual void displayScannerEncoderValues(const int* risingValues, const int* fallingValues) = 0;
    
    // 显示系统仪表盘
    virtual void displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) = 0;
    
    // 显示正常模式直径信息
    virtual void displayNormalModeDiameter(int latestDiameter) = 0;
    
    // 显示正常模式统计信息（兼容旧方法）
    virtual void displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) = 0;
    
    // 显示速度统计信息
    virtual void displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) = 0;
    
    // 显示单个值
    virtual void displaySingleValue(const String& label, int value, const String& unit) = 0;
    
    // 显示位置信息
    virtual void displayPositionInfo(const String& title, int position, bool showOnlyOnChange) = 0;
    
    // 显示诊断值
    virtual void displayDiagnosticValues(const String& title, const String& value1, const String& value2) = 0;
    
    // 显示多行文本
    virtual void displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3 = "") = 0;
    
    // 重置诊断模式
    virtual void resetDiagnosticMode() = 0;
};

#endif // DISPLAY_H
