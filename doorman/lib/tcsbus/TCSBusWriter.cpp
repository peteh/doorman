#include "TCSBus.h"

volatile bool TCSBusWriter::s_writing = false;

TCSBusWriter::TCSBusWriter(uint8_t writePin)
:m_writePin(writePin)
{
}

void TCSBusWriter::begin()
{
    pinMode(m_writePin, OUTPUT);
}

bool TCSBusWriter::isWriting()
{
    return s_writing;
}

void TCSBusWriter::write(uint32_t data)
{
    TCSBusWriter::s_writing = true;
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
    TCSBusWriter::s_writing = false;
}