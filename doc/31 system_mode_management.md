# 系统工作模式管理

## 1. 工作模式
- **MODE_NORMAL**: 正常工作模式，执行完整分拣流程
- **MODE_DIAGNOSE_ENCODER**: 编码器诊断模式
- **MODE_DIAGNOSE_SCANNER**: 扫描仪诊断模式
- **MODE_DIAGNOSE_OUTLET**: 出口诊断模式
- **MODE_DIAGNOSE_CONVEYOR**: 传输线诊断模式
- **MODE_TEST**: 系统测试模式

## 2. 模式切换
通过主按钮切换工作模式，使用switch case结构实现