#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include "../../main.h"
#include "../common/display.h"  // 包含Display抽象基类头文件

// 应用程序类型前向声明
enum AppType;

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
  uint32_t previousUpdateTime;
  
  // 显示更新间隔（毫秒）
  const uint32_t UPDATE_INTERVAL = 2000;
  
  // 终端显示样式常量定义
  // 颜色编号说明：
  // 背景色：40=黑, 41=红, 42=绿, 43=黄, 44=蓝, 45=紫, 46=青, 47=白
  // 前景色：30=黑, 31=红, 32=绿, 33=黄, 34=蓝, 35=紫, 36=青, 37=白
  // 使用方式：\033[背景色;前景色m
  const String STYLE_RESET = "";              // 去掉转义
  const String STYLE_DATA_WINDOW_TITLE = "";  // 去掉转义
  const String STYLE_DATA_WINDOW_CONTENT = "";  // 去掉转义
  const String STYLE_NOTIFICATION = "";  // 去掉转义
  
  // 存储上一次显示的数据，用于检测变化
  AppType previousDisplayedAppType;  // 上一次显示的应用程序类型
  int previousEncoderPosition;  // 上一次编码器位置
  float previousSortingSpeedPerSecond;  // 上一次显示的每秒速度
  int previousSortingSpeedPerMinute;  // 上一次显示的每分钟速度
  int previousSortingSpeedPerHour;  // 上一次显示的每小时速度
  int previousIdentifiedCount;  // 上一次显示的已识别数量
  int previousTransportedTrayCount;  // 上一次显示的已输送托盘数量
  int previousLatestDiameter;  // 上一次显示的最新直径
  int previousLatestScanCount; // 上一次显示的最新根数
  
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
  
  // 核心接口：刷新快照
  void refresh(const DisplaySnapshot& snapshot) override;

  // 菜单渲染代理
  void renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) override;
  
  // 清理屏幕 (Terminal可以用换行或清屏指令)
  void clearDisplay() override;
};

#endif // TERMINAL_H
