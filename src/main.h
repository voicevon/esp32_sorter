#ifndef MAIN_H
#define MAIN_H

// 系统工作模式定义
enum SystemMode {
  MODE_NORMAL = 0,            // 正常工作模式
  MODE_DIAGNOSE_ENCODER = 1,  // 诊断编码器模式
  MODE_DIAGNOSE_SCANNER = 2,  // 诊断扫描仪模式
  MODE_DIAGNOSE_OUTLET = 3,   // 诊断出口模式
  MODE_TEST_RELOADER = 4      // 上料器测试模式（Feeder Test Mode）
};

#endif // MAIN_H