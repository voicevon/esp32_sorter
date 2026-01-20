#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include "main.h"
#include "user_interface/display.h"  // 包含Display抽象基类头文件

// 系统工作模式前向声明
enum SystemMode;

/**
 * @class Terminal
 * @brief 串口终端输出管理类
 * 
 * 该类使用单例模式实现，负责管理通过串口输出的所有文本信息
 * 提供与OLED类类似的接口，实现统一的输出管理
 */
class Terminal : public Display {
private:
  // 单例模式的私有构造函数
  Terminal();
  
  // 防止拷贝
  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;
  
  // 静态实例指针
  static Terminal* instance;
  
  // 上一次更新时间
  unsigned long previousUpdateTime;
  
  // 显示更新间隔（毫秒）
  const unsigned long UPDATE_INTERVAL = 2000;
  
  // 终端显示样式常量定义
  // 颜色编号说明：
  // 背景色：40=黑, 41=红, 42=绿, 43=黄, 44=蓝, 45=紫, 46=青, 47=白
  // 前景色：30=黑, 31=红, 32=绿, 33=黄, 34=蓝, 35=紫, 36=青, 37=白
  // 使用方式：\033[背景色;前景色m
  const String STYLE_RESET = "\033[0m";              // 重置所有样式
  const String STYLE_DATA_WINDOW_TITLE = "\033[44m\033[31m";  // 数据窗口标题：蓝底(44)红(31)字
  const String STYLE_DATA_WINDOW_CONTENT = "\033[44m\033[37m";  // 数据窗口内容：蓝底(44)白(37)字
  const String STYLE_NOTIFICATION = "\033[43m\033[32m";  // 通知消息：黄底(43)绿(32)字
  
  // 存储上一次显示的数据，用于检测变化
  SystemMode previousDisplayedMode;  // 上一次显示的模式
  int previousEncoderPosition;  // 上一次编码器位置
  float previousSortingSpeedPerSecond;  // 上一次显示的每秒速度
  int previousSortingSpeedPerMinute;  // 上一次显示的每分钟速度
  int previousSortingSpeedPerHour;  // 上一次显示的每小时速度
  int previousIdentifiedCount;  // 上一次显示的已识别数量
  int previousTransportedTrayCount;  // 上一次显示的已输送托盘数量
  int previousLatestDiameter;  // 上一次显示的最新直径
  
  // 辅助方法
  String translate(const String& key) const;
  bool isUpdateReady() const;
  
public:
  // 单例模式的获取实例方法
  static Terminal* getInstance();
  
  // 初始化串口终端
  void initialize() override;
  
  // 检查终端是否可用
  bool isAvailable() const override { return true; }  // 串口始终可用
  
  // 显示模式变化信息
  void displayModeChange(SystemMode newMode) override;
  void displayModeChange(const String& newModeName) override;
  
  // 显示出口状态变化
  void displayOutletStatus(uint8_t outletIndex, bool isOpen) override;
  
  // 显示诊断信息
  void displayDiagnosticInfo(const String& title, const String& info) override;
  
  // 显示出口测试模式图形
  void displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode) override;
  
  // 显示出口寿命测试专用图形
  void displayOutletLifetimeTestGraphic(uint8_t outletCount, unsigned long cycleCount, bool outletState, int subMode) override;
  
  // 显示扫描仪编码器值
  void displayScannerEncoderValues(const int* risingValues, const int* fallingValues) override;
  
  // 显示系统仪表盘
  void displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) override;
  
  // 显示直径信息（功能专用方法）
  void displayDiameter(int latestDiameter) override;
  
  // 显示正常模式直径信息（兼容旧接口）
  void displayNormalModeDiameter(int latestDiameter) override;
  
  // 显示正常模式统计信息（兼容旧接口）
  void displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount) override;
  
  // 显示速度统计信息
  void displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount) override;
  
  // 显示单个值
  void displaySingleValue(const String& label, int value, const String& unit) override;
  
  // 显示位置信息
  void displayPositionInfo(const String& title, int position, bool showOnlyOnChange) override;
  
  // 显示诊断值
  void displayDiagnosticValues(const String& title, const String& value1, const String& value2) override;
  
  // 显示多行文本
  void displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3 = "") override;
  
  // 重置诊断模式
  void resetDiagnosticMode() override;
};

#endif // TERMINAL_H
