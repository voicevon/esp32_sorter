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
        if (pinOpen >= 0) {
            pinMode(pinOpen, OUTPUT);
            digitalWrite(pinOpen, LOW);
        }
        if (pinClose >= 0) {
            pinMode(pinClose, OUTPUT);
            digitalWrite(pinClose, LOW);
        }
        // Force close on init
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

        // If desired state matches physical state, do nothing
        if (readyToOpenState == physicalOpen && !isPulsing) {
            return;
        }

        // If we are already pulsing towards the target, do nothing
        if (isPulsing && targetPulseState == readyToOpenState) {
            return; 
        }

        // Start new action
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
        if (pinOpen >= 0) digitalWrite(pinOpen, HIGH);
        if (pinClose >= 0) digitalWrite(pinClose, LOW);
        
        isPulsing = true;
        pulseStateChangeTime = millis();
        targetPulseState = true;
        physicalOpen = true; // Optimistically set state
    }

    void executeClose() {
        if (pinOpen >= 0) digitalWrite(pinOpen, LOW);
        if (pinClose >= 0) digitalWrite(pinClose, HIGH);
        
        isPulsing = true;
        pulseStateChangeTime = millis();
        targetPulseState = false; // Closing
        physicalOpen = false;
    }

    void stopPulse() {
        if (pinOpen >= 0) digitalWrite(pinOpen, LOW);
        if (pinClose >= 0) digitalWrite(pinClose, LOW);
        isPulsing = false;
    }
};

#endif // OUTLET_H
