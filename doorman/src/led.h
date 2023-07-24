#pragma once
#include <Arduino.h>

class Led
{
public:

    virtual void begin() = 0;
    virtual void blinkAsync() = 0;

    virtual void setBackgroundLight(bool enabled) = 0;

    virtual void update() = 0;
};