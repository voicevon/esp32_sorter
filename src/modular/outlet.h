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
          initialized(false), 
          readyToOpenState(false), 
          matchDiameterMin(0), 
          matchDiameterMax(0) {}

    /**
     * 初始化出口逻辑状态
     */
    void initialize() {
        // 逻辑初始化：强制设定为关闭位置
        executeClose();
        initialized = true;
    }

    /**
     * 更新脉冲时序（由 Sorter 定时调用）
     */
    void update() {
        if (isPulsing) {
            if (millis() - pulseStateChangeTime >= PULSE_DURATION) {
                stopPulse();
            }
        }
    }

    /**
     * 执行目标动作（当 readyToOpenState 改变时触发）
     */
    void execute() {
        if (!initialized) return;

        // 如果目标位置与当前物理留驻位置一致且没有正在发出的脉冲，则忽略
        if (readyToOpenState == physicalOpen && !isPulsing) {
            return;
        }

        // 如果已经在向目标方向发送脉冲，则维持当前状态
        if (isPulsing && targetPulseState == readyToOpenState) {
            return; 
        }

        // 根据逻辑目标启动 H 桥换向脉冲
        if (readyToOpenState) {
            executeOpen();
        } else {
            executeClose();
        }
    }

    void setReadyToOpen(bool state) {
        readyToOpenState = state;
    }

    bool isReadyToOpen() const {
        return readyToOpenState;
    }

    /** 
     * 获取物理位置（对应指示 LED）
     */
    bool isPositionOpen() const {
        return physicalOpen;
    }

    /**
     * 获取吸合脉冲状态（对应 H 桥 A 通道）
     */
    bool isOpenPulseActive() const {
        return isPulsing && targetPulseState;
    }

    /**
     * 获取释放脉冲状态（对应 H 桥 B 通道）
     */
    bool isClosePulseActive() const {
        return isPulsing && !targetPulseState;
    }

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
    bool initialized;
    bool readyToOpenState;        // 软件下达的目标逻辑状态

    int matchDiameterMin;
    int matchDiameterMax;

    void executeOpen() {
        isPulsing = true;
        pulseStateChangeTime = millis();
        targetPulseState = true;
        physicalOpen = true; 
    }

    void executeClose() {
        isPulsing = true;
        pulseStateChangeTime = millis();
        targetPulseState = false; 
        physicalOpen = false;
    }

    void stopPulse() {
        isPulsing = false;
    }
};

#endif // OUTLET_H
