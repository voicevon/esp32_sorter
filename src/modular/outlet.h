#ifndef OUTLET_H
#define OUTLET_H

#include <Arduino.h>

class Outlet {
private:
    int pinOpen;
    int pinClose;
    bool physicalOpen;
    unsigned long pulseStateChangeTime;
    bool isPulsing;
    bool targetPulseState; // true=Opening, false=Closing
    const unsigned long PULSE_DURATION = 500;
    
    bool initialized;
    bool readyToOpenState;
    int matchDiameterMin;
    int matchDiameterMax;

public:
    Outlet(int openPin, int closePin) 
        : pinOpen(openPin), pinClose(closePin), physicalOpen(false), 
          pulseStateChangeTime(0), isPulsing(false), targetPulseState(false),
          initialized(false), readyToOpenState(false), matchDiameterMin(0), matchDiameterMax(0) {}

    void initialize() {
        // 彻底移除 direct GPIO 控制逻辑，后续由 Sorter 统一推向 HC595
        executeClose();
        initialized = true;
    }

    void update() {
        if (isPulsing) {
            if (millis() - pulseStateChangeTime >= PULSE_DURATION) {
                stopPulse();
            }
        }
    }

    void execute() {
        if (!initialized) return;

        // 如果目标状态与物理状态一致且没有在脉冲中，则无需动作
        if (readyToOpenState == physicalOpen && !isPulsing) {
            return;
        }

        // 如果正在朝着目标执行脉冲工作，也保持原样
        if (isPulsing && targetPulseState == readyToOpenState) {
            return; 
        }

        // 启动逻辑动作
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
     * 获取逻辑位置状态（对应 595 Index 0）
     */
    bool isPositionOpen() const {
        return physicalOpen;
    }

    /**
     * 获取 H 桥吸合脉冲输出请求（对应 595 Index 1）
     */
    bool isOpenPulseActive() const {
        return isPulsing && targetPulseState;
    }

    /**
     * 获取 H 桥释放脉冲输出请求（对应 595 Index 2）
     */
    bool isClosePulseActive() const {
        return isPulsing && !targetPulseState;
    }

    void setMatchDiameter(int min, int max) {
        matchDiameterMin = min;
        matchDiameterMax = max;
    }

    int getMatchDiameterMin() const { return matchDiameterMin; }
    int getMatchDiameterMax() const { return matchDiameterMax; }

    void setMatchDiameterMin(int min) { matchDiameterMin = min; }
    void setMatchDiameterMax(int max) { matchDiameterMax = max; }


private:
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
