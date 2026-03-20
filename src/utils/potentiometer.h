#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <Arduino.h>

class Potentiometer {
public:
    Potentiometer(int pin, int windowSize = 10);
    ~Potentiometer();

    void initialize();
    void update();
    int getSmoothedValue();
    int getMappedValue(int outMin, int outMax);

private:
    int _pin;
    int _windowSize;
    int* _samples;
    int _index;
    long _sum;
    bool _bufferFull;
};

#endif
