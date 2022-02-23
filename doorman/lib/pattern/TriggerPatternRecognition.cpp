#include "TriggerPatternRecognition.h"
#include <limits.h>

TriggerPatternRecognition::TriggerPatternRecognition()
    : m_patternIndex(0),
      m_patternStepCount(0),
      m_lastTriggerMs(ULONG_MAX / 2) // use an arbitrary value that is hard to accidently hit
{
}

void TriggerPatternRecognition::addStep(uint32_t timeMs)
{
    if (m_patternStepCount >= MAX_STEPS)
    {
        // TODO: throw exception?
        return;
    }
    m_pattern[m_patternStepCount++] = timeMs;
}

unsigned long TriggerPatternRecognition::calcInaccuracy(unsigned long deltaMs)
{
    float inaccuracyFactor = 0.3; // 0.x*100 % of deviation up or down in time meassure
    return inaccuracyFactor * deltaMs;
}

bool TriggerPatternRecognition::trigger()
{
    if (m_patternStepCount == 0)
    {
        // no pattern defined
        return false;
    }

    // calculate time since last press
    unsigned long now = millis();
    unsigned long actualDeltaMs = now - m_lastTriggerMs;
    m_lastTriggerMs = now;

    unsigned long expectedDeltaMs = m_pattern[m_patternIndex];
    unsigned long inaccuracyMs = calcInaccuracy(expectedDeltaMs);
    
    // if last trigger till now is within our pattern we jump to next step
    if (actualDeltaMs > expectedDeltaMs - inaccuracyMs &&
        actualDeltaMs < expectedDeltaMs + inaccuracyMs)
    {
        Serial.printf("Pattern step passed (plan: %lums, measured: %lums, allowed inaccuracy %lums)",
                      expectedDeltaMs,
                      actualDeltaMs,
                      inaccuracyMs);
        Serial.println();
        m_patternIndex++;
    }
    else
    {
        // if not we reset the pattern
        Serial.printf("Pattern step failed, resetting pattern (plan: %lums, measured: %lums, allowed inaccuracy %lums)",
                      expectedDeltaMs,
                      actualDeltaMs,
                      inaccuracyMs);
        Serial.println();
        m_patternIndex = 0;
        return false;
    }

    // if we reached the end, then we should trigger and reset to first step again
    if (m_patternIndex >= m_patternStepCount)
    {
        Serial.println("Pattern recognized");
        m_patternIndex = 0;
        return true;
    }

    // if we are not at the end of the pattern we just return false
    return false;
}
