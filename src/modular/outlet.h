#ifndef OUTLET_H
#define OUTLET_H

#include <Arduino.h>

class Outlet {
private:
public:
    Outlet() : 
          isPulsing(false), 
          pulseStateChangeTime(0), 
          targetPulseState(false), 
          physicalOpen(false), 
          readyToOpenState(false), 
          stayOpenNext(false),
          matchDiameterMin(0), 
          matchDiameterMax(0),
          targetLength(0) {} // 0: ANY, 1: S, 2: M, 3: L

    void initialize();
    void update();
    void execute();

    // 预见性控制接口
    void setStayOpenNext(bool stay) { stayOpenNext = stay; }
    bool shouldStayOpenNext() const { return stayOpenNext; }

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
    
    // 长度限定配置 (0:ANY, 1:S, 2:M, 3:L)
    void setTargetLength(uint8_t len) { targetLength = len; }
    uint8_t getTargetLength() const { return targetLength; }

private:
    bool isPulsing;               // 正在发送高电平脉冲
    unsigned long pulseStateChangeTime; // 脉冲起始时间
    bool targetPulseState;        // 当前脉冲的目标方向（true=吸合, false=释放）
    bool physicalOpen;            // 电磁铁物理留驻位置（true=Open, false=Close）
    bool readyToOpenState;        // 软件下达的目标逻辑状态

    int matchDiameterMin;
    int matchDiameterMax;
    bool stayOpenNext;            // 预见性：标记下一个托盘是否也需要进此洞
    uint8_t targetLength;         // 0: ANY, 1: S, 2: M, 3: L

    void executeOpen();
    void executeClose();
    void stopPulse();
};

#endif // OUTLET_H
