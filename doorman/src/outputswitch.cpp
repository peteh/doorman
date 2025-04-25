#include "outputswitch.h"

void OutputSwitch::begin()
{
    pinMode(m_pin, OUTPUT);
}

void OutputSwitch::loop()
{
    if (!m_enabled || m_timeout == 0)
    {
        return;
    }

    if (millis() - m_lastSwitch > m_timeout)
    {
        // disable after timeout
        switchto(false);
    }
}

void OutputSwitch::setNotificationCallback(std::function<void(bool, uint32_t)> callback)
{
    m_notificationCallback = callback;
}

void OutputSwitch::switchto(bool enabled)
{
    if (m_enabled == enabled && m_enabled == digitalRead(m_pin))
    {
        return;
    }
    m_lastSwitch = millis();
    m_enabled = enabled;
    digitalWrite(m_pin, m_enabled ? HIGH : LOW);

    if (m_notificationCallback)
    {
        m_notificationCallback(m_enabled, m_timeout);
    }
}