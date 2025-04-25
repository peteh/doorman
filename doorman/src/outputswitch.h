#pragma once
#include <Arduino.h>

class OutputSwitch
{
public: 
    OutputSwitch(uint8_t pin)
    : m_pin(pin)
    {

    }
    
    void begin();
    void switchto(bool enabled);
    void loop();
    void setNotificationCallback(std::function<void(bool, uint32_t)> callback);
    void setTimeout(uint32_t timeout)
    {
        m_timeout = timeout;
    }

private: 
    uint8_t m_pin;
    uint32_t m_timeout = 0;
    long m_lastSwitch = 0;
    bool m_enabled = false;
    std::function<void(bool, uint32_t)> m_notificationCallback = nullptr;
};