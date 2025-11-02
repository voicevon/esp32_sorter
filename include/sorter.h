#ifndef SORTER_H
#define SORTER_H

#include "carriage_system.h"
#include "encoder.h"
#include "outlet.h"

class Sorter {
public:
    Sorter();
    ~Sorter();
    
    // 初始化函数
    void initialize();
    
    // 主循环调用的更新函数
    void spin_Once();
    
    // 启动分拣系统
    void start();
    
    // 停止分拣系统
    void stop();
    
    // 重置分拣系统
    void reset();
    
    // 接收直径数据
    void receiveDiameterData(float diameter);
    
private:
    Encoder encoder;
    Outlet outlet1;
    Outlet outlet2;
    Outlet outlet3;
    Outlet outlet4;
    Outlet outlet5;
    
    bool isRunning;
    bool isInitialized;
    
    // 处理编码器位置变化
    void handlePositionChange(int position);
    
    // 控制出口动作
    void controlOutlets(int position);
};

#endif // SORTER_H