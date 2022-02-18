#pragma once
#include <Arduino.h>

#define START_BIT 6
#define ONE_BIT 4
#define ZERO_BIT 2

class TCSBusReader
{
public:
    TCSBusReader(uint8_t readPin);
    void begin();
    bool hasCommand();
    void setEnabled(bool enabled);
    uint32_t read();

private:
    static bool m_enabled;
};

class TCSBusWriter
{
public:
    /**
     * @brief Allows to write to the TCS Bus.
     *
     * @param writePin the pin on which the bus cable is connected.
     */
    TCSBusWriter(uint8_t writePin);

    /**
     * @brief Must be called in the setup phase.
     *
     */
    void begin();

    /**
     * @brief Writes the data to the bus. Can be a short command with 16 bits or a
     * long commands with 32.
     * Example codes:
     *   0x1100 door opener if the handset is not lifted up (short)
     *   0x1180 door opener if the handset is lifted up (short)
     *   0x1B8F9A41 specific bell code for a door (long)
     * @param data the data to write to the bus
     */
    void write(uint32_t data);

private:
    uint8_t m_writePin;
};