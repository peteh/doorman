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
    // this is magic from https://github.com/atc1441/TCSintercomArduino
    TCSBusWriter::s_writing = true;
    int length = 16;
    byte checksm = 1;
    bool isLongMessage = false;
    if (data > 0xFFFF)
    {
        length = 32;
        isLongMessage = 1;
    }
    digitalWrite(m_writePin, HIGH);
    delay(TCS_MSG_START_MS);
    digitalWrite(m_writePin, !digitalRead(m_writePin));
    delay(isLongMessage ? TCS_ONE_BIT_MS : TCS_ZERO_BIT_MS);
    int curBit = 0;
    for (byte i = length; i > 0; i--)
    {
        curBit = bitRead(data, i - 1);
        digitalWrite(m_writePin, !digitalRead(m_writePin));
        delay(curBit ? TCS_ONE_BIT_MS : TCS_ZERO_BIT_MS);
        checksm ^= curBit;
    }
    digitalWrite(m_writePin, !digitalRead(m_writePin));
    delay(checksm ? TCS_ONE_BIT_MS : TCS_ZERO_BIT_MS);
    digitalWrite(m_writePin, LOW);
    TCSBusWriter::s_writing = false;
}