#ifndef OUTLET_H
#define OUTLET_H

#include <Arduino.h>

class Outlet {
private:
public:
    // 脉冲持续时间（毫秒），确保 H 桥动作后能够及时断电
    static const unsigned long PULSE_DURATION = 500;

    Outlet() : 
          isPulsing(false), 
          pulseStateChangeTime(0), 
          targetPulseState(false), 
          physicalOpen(false), 
          readyToOpenState(false), 
          matchDiameterMin(0), 
          matchDiameterMax(0) {}

    void initialize();
    void update();
    void execute();

    void setReadyToOpen(bool state) { readyToOpenState = state; }
    bool isReadyToOpen() const { return readyToOpenState; }
    bool isPositionOpen() const { return physicalOpen; }
    
    bool isOpenPulseActive() const { return isPulsing && targetPulseState; }
    bool isClosePulseActive() const { return isPulsing && !targetPulseState; }

    // 直径匹配配置
    void setMatchDiameter(int min, int max) {
        matchDiameterMin = min;
        matchDiameterMax = max;
    }

    int getMatchDiameterMin() const { return matchDiameterMin; }
    int getMatchDiameterMax() const { return matchDiameterMax; }

    void setMatchDiameterMin(int min) { matchDiameterMin = min; }
    void setMatchDiameterMax(int max) { matchDiameterMax = max; }

private:
    bool isPulsing;               // 正在发送高电平脉冲
    unsigned long pulseStateChangeTime; // 脉冲起始时间
    bool targetPulseState;        // 当前脉冲的目标方向（true=吸合, false=释放）
    bool physicalOpen;            // 电磁铁物理留驻位置（true=Open, false=Close）
    bool readyToOpenState;        // 软件下达的目标逻辑状态

    int matchDiameterMin;
    int matchDiameterMax;

    void executeOpen();
    void executeClose();
    void stopPulse();
};

#endif // OUTLET_H
