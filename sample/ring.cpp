#include <Arduino.h>

#define outputPin D5
#define startBit 6
#define oneBit 4
#define zeroBit 2

/*Protokoll:
   bit
   0     = length: 0 = 2 Bytes 1 = 4 Bytes  everytime plus 1 crc
   1-8   = first byte
   9-16  = second byte
     now 1 bit crc xor OR the next two byte
   17-24 = third byte
   25-32 = last byte
     now 1 crc bit if first bit was 1
   6ms delay start bit
   4m delay 1 bit
   2ms delay 0 bit
*/

//const uint32_t Klingeln = 0x0361987F; // serial: 36198 HEX = 221592 change to your seriall.
const uint32_t Klingeln = 0x1B8F9A41; // serial: 36198 HEX = 221592 change to your seriall.

bool ledCounter = false;

void sendToBus(uint32_t protokoll)
{
    int length = 16;
    byte checksm = 1;
    byte firstBit = 0;
    if (protokoll > 0xFFFF)
    {
        length = 32;
        firstBit = 1;
    }
    digitalWrite(outputPin, HIGH);
    delay(startBit);
    digitalWrite(outputPin, !digitalRead(outputPin));
    delay(firstBit ? oneBit : zeroBit);
    int curBit = 0;
    for (byte i = length; i > 0; i--)
    {
        curBit = bitRead(protokoll, i - 1);
        digitalWrite(outputPin, !digitalRead(outputPin));
        delay(curBit ? oneBit : zeroBit);
        checksm ^= curBit;
    }
    digitalWrite(outputPin, !digitalRead(outputPin));
    delay(checksm ? oneBit : zeroBit);
    digitalWrite(outputPin, LOW);
}

void setup()
{
    pinMode(outputPin, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    delay(1000);
    
    sendToBus(Klingeln);
}

void loop()
{
  digitalWrite(LED_BUILTIN, ledCounter);
  ledCounter = !ledCounter;
  delay(2000);
}