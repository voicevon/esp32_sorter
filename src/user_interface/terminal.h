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
  
  // 最后更新时间
  unsigned long lastUpdateTime;
  
  // 显示更新间隔（毫秒）
  const unsigned long UPDATE_INTERVAL = 2000;
  
  // 存储上一次显示的数据，用于检测变化
  SystemMode lastDisplayedMode;  // 上一次显示的模式
  int lastEncoderPosition;  // 上一次编码器位置
  float lastSortingSpeedPerSecond;  // 上一次显示的每秒速度
  int lastSortingSpeedPerMinute;  // 上一次显示的每分钟速度
  int lastSortingSpeedPerHour;  // 上一次显示的每小时速度
  int lastIdentifiedCount;  // 上一次显示的已识别数量
  int lastTransportedTrayCount;  // 上一次显示的已输送托盘数量
  int lastLatestDiameter;  // 上一次显示的最新直径
  
  // 辅助方法
  String translate(const String& key) const;
  bool isUpdateReady() const;
  
public:
  // 单例模式的获取实例方法
  static Terminal* getInstance();
  
  // 初始化串口终端
  void initialize();
  
  // 检查终端是否可用
  bool isAvailable() const { return true; }  // 串口始终可用
  
  // 显示模式变化信息
  void displayModeChange(SystemMode newMode);
  void displayModeChange(const String& newModeName);
  
  // 显示出口状态变化
  void displayOutletStatus(uint8_t outletIndex, bool isOpen);
  
  // 显示诊断信息
  void displayDiagnosticInfo(const String& title, const String& info);
  
  // 显示出口测试模式图形
  void displayOutletTestGraphic(uint8_t outletCount, uint8_t openOutlet, int subMode);
  
  // 显示扫描仪编码器值
  void displayScannerEncoderValues(const int* risingValues, const int* fallingValues);
  
  // 显示系统仪表盘
  void displayDashboard(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount);
  
  // 显示正常模式直径信息
  void displayNormalModeDiameter(int latestDiameter);
  
  // 显示正常模式统计信息（旧方法，保持兼容）
  void displayNormalModeStats(float sortingSpeedPerSecond, int sortingSpeedPerMinute, int sortingSpeedPerHour, int identifiedCount, int transportedTrayCount);
  
  // 显示速度统计信息
  void displaySpeedStats(int speedPerSecond, int speedPerMinute, int speedPerHour, int itemCount, int trayCount);
  
  // 显示单个值
  void displaySingleValue(const String& label, int value, const String& unit);
  
  // 显示位置信息
  void displayPositionInfo(const String& title, int position, bool showOnlyOnChange);
  
  // 显示诊断值
  void displayDiagnosticValues(const String& title, const String& value1, const String& value2);
  
  // 显示多行文本
  void displayMultiLineText(const String& title, const String& line1, const String& line2, const String& line3 = "");
  
  // 重置诊断模式
  void resetDiagnosticMode();
};

#endif // TERMINAL_H
