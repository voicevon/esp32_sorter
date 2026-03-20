#include "potentiometer.h"

Potentiometer::Potentiometer(int pin, int windowSize) 
    : _pin(pin), _windowSize(windowSize), _index(0), _sum(0), _bufferFull(false) {
    _samples = new int[windowSize];
    for (int i = 0; i < windowSize; i++) _samples[i] = 0;
}

Potentiometer::~Potentiometer() {
    delete[] _samples;
}

void Potentiometer::initialize() {
    pinMode(_pin, INPUT);
}

void Potentiometer::update() {
    int raw = analogRead(_pin);
    _sum -= _samples[_index];
    _samples[_index] = raw;
    _sum += raw;
    _index = (_index + 1) % _windowSize;
    if (_index == 0) _bufferFull = true;
}

int Potentiometer::getSmoothedValue() {
    if (!_bufferFull && _index > 0) return _sum / _index;
    if (!_bufferFull) return 0;
    return _sum / _windowSize;
}

int Potentiometer::getMappedValue(int outMin, int outMax) {
    return map(getSmoothedValue(), 0, 4095, outMin, outMax);
}
