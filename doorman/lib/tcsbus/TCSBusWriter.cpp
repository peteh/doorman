#include "TCSBus.h"

TCSBusWriter::TCSBusWriter(uint8_t writePin)
:m_writePin(writePin)
{
}

void TCSBusWriter::begin()
{
    pinMode(m_writePin, OUTPUT);
}

void TCSBusWriter::write(uint32_t data)
{
    int length = 16;
    byte checksm = 1;
    byte firstBit = 0;
    if (data > 0xFFFF)
    {
        length = 32;
        firstBit = 1;
    }
    digitalWrite(m_writePin, HIGH);
    delay(START_BIT);
    digitalWrite(m_writePin, !digitalRead(m_writePin));
    delay(firstBit ? ONE_BIT : ZERO_BIT);
    int curBit = 0;
    for (byte i = length; i > 0; i--)
    {
        curBit = bitRead(data, i - 1);
        digitalWrite(m_writePin, !digitalRead(m_writePin));
        delay(curBit ? ONE_BIT : ZERO_BIT);
        checksm ^= curBit;
    }
    digitalWrite(m_writePin, !digitalRead(m_writePin));
    delay(checksm ? ONE_BIT : ZERO_BIT);
    digitalWrite(m_writePin, LOW);
}