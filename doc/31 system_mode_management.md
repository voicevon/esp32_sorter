# 系统工作模式管理

## 1. 工作模式
- **MODE_NORMAL**: 正常工作模式，执行完整分拣流程（扫描、计算、分拣）
- **MODE_DIAGNOSE_ENCODER**: 编码器诊断模式，显示编码器相位变化
- **MODE_DIAGNOSE_SCANNER**: 扫描仪诊断模式，显示传感器原始数据
- **MODE_DIAGNOSE_OUTLET**: 出口诊断模式，测试各出口舵机动作
- **MODE_DIAGNOSE_CONVEYOR**: 传输线诊断模式，控制传输线运行
- **MODE_TEST**: 系统测试模式，测试整体系统功能

## 2. 模式切换
- 通过SimpleHMI模块的主按钮实现模式切换
- 短按主按钮切换到下一个工作模式
- 每种模式对应不同的LED指示灯状态
- 使用switch-case结构在main.cpp中实现模式切换逻辑