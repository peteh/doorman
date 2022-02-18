#pragma once
#include <Arduino.h>

class TriggerPatternRecognition
{
public:
    TriggerPatternRecognition();
    void addStep(uint32_t timeMs);
    bool trigger();

private:
    unsigned long calcInaccuracy(unsigned long deltaMs);

    static const uint8_t MAX_STEPS = 10;

    uint32_t m_pattern[MAX_STEPS];
    uint8_t m_patternIndex;
    uint8_t m_patternStepCount;

    unsigned long m_lastTriggerMs;
};
