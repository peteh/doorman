## Table of Contents

- [Introduction](#introduction)
- [Class](#class)
    - [Constructor](#constructor)
    - [Methods](#methods)
- [Example Usage](#example-usage)

## Introduction
The `TriggerPatternRecognition` class is a C++ class that can be used to recognize a sequence of button presses based on the time between each press. This class can be used to implement a variety of features, such as a secret knock or a Morse code decoder.

## Class

### Constructor

```cpp
TriggerPatternRecognition();
```

The constructor initializes the class with the following default values:

- `m_patternIndex`: 0
- `m_patternStepCount`: 0
- `m_lastTriggerMs`: ULONG_MAX / 2

### Methods

- `addStep(uint32_t timeMs)`
  - Adds a step to the trigger pattern.
  - The timeMs parameter specifies the time in milliseconds since the last step.
  - If the pattern step count is greater than or equal to MAX_STEPS, the method does nothing.
- `calcInaccuracy(unsigned long deltaMs)`
  - Calculates the inaccuracy of a time delta.
  - The deltaMs parameter specifies the time delta in milliseconds.
  - The method returns the inaccuracy in milliseconds.
- `trigger()`
  - Tries to trigger the pattern.
  - The method returns true if the pattern is triggered, and false otherwise.

## Example Usage

```cpp
#include "TriggerPatternRecognition.h"

int main() {
  TriggerPatternRecognition patternRecognition;

  // Add the steps to the pattern.
  patternRecognition.addStep(100);
  patternRecognition.addStep(200);
  patternRecognition.addStep(300);

  // Try to trigger the pattern.
  if (patternRecognition.trigger()) {
    // The pattern was triggered.
  } else {
    // The pattern was not triggered.
  }

  return 0;
}
```