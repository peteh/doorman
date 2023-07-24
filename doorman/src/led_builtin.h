#pragma once
#include <Arduino.h>

#include "led.h"

class LedBuiltin : public Led
{
public:
    LedBuiltin(uint8_t outputPin)
        : m_output(outputPin)
    {
    }

    virtual void begin() override
    {
        pinMode(m_output, OUTPUT);
    }

    virtual void blinkAsync() override
    {
        m_lastOn = millis();
        m_ledState = true;
        digitalWrite(LED_BUILTIN, LOW ^ m_backgroundEnabled);
    }

    virtual void setBackgroundLight(bool enabled) override
    {
        if(enabled)
        {
            m_backgroundEnabled = true;
            digitalWrite(m_output, LOW);
        }
        else
        {
            m_backgroundEnabled = false;
            digitalWrite(m_output, HIGH);
        }
    }

    virtual void update() override
    {
        if (m_ledState)
        {
            if (millis() - m_lastOn > 50)
            {
                m_ledState = false;
            // we invert with party mode to make the led constantly light if it's enabled
            digitalWrite(LED_BUILTIN, HIGH ^ m_backgroundEnabled);
            }
        }
    }

private:
    uint8_t m_output;
    long m_lastOn = 0;
    bool m_ledState = false;
    bool m_backgroundEnabled = false;
};