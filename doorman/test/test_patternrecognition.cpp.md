## 📝 Trigger Pattern Recognition 📝

Trigger Pattern Recognition is a library that allows you to create and trigger patterns of events.
It is useful for creating custom event-based triggers that can be used to control devices or processes.

- 📘 **Table of Contents** 📘
  - [Features](#Features)
  - [Installation](#Installation)
  - [Usage](#Usage)
    - [Creating a Pattern](#Creating-a-Pattern)
    - [Triggering a Pattern](#Triggering-a-Pattern)
  - [Example](#Example)
  - [Contributing](#Contributing)

### 🧰 Features 🧰

- Create patterns of events
- Trigger events based on patterns

### 📦 Installation 📦

To install the Trigger Pattern Recognition library, add the following line to your `Arduino.ino` file:

```c++
#include <TriggerPatternRecognition.h>
```

### 🕹️ Usage 🕹️

To use the Trigger Pattern Recognition library, you first need to create a pattern. A pattern is a sequence of events.
Each event is specified by a time delay.
For example, the following code creates a pattern that waits for 2 seconds, then waits for 3 seconds:

```c++
TriggerPatternRecognition recognition;
recognition.addStep(2000);
recognition.addStep(3000);
```

Once you have created a pattern, you can trigger it by calling the `trigger()` function.
The `trigger()` function will return true if the pattern is successful.
For example, the following code checks if the pattern is successful:

```c++
if (recognition.trigger()) {
  // Pattern was successful
}
```

### 💡 Example 💡

The following example shows how to use the Trigger Pattern Recognition library to create a custom event-based trigger that controls an LED:

```c++
#include <TriggerPatternRecognition.h>
#include <Arduino.h>

TriggerPatternRecognition recognition;

void setup() {
  // Create a pattern that waits for 2 seconds, then waits for 3 seconds
  recognition.addStep(2000);
  recognition.addStep(3000);

  // Set the LED to output
  pinMode(BUILTIN_LED, OUTPUT);
}

void loop() {
  // Check if the pattern is successful
  if (recognition.trigger()) {
    // Turn on the LED
    digitalWrite(BUILTIN_LED, HIGH);
  } else {
    // Turn off the LED
    digitalWrite(BUILTIN_LED, LOW);
  }
}
```

In this example, the LED will turn on when the pattern is successful.
The pattern is successful when the LED has been on for 2 seconds, then off for 3 seconds.

### 👉 Contributing 👉

Contributions are welcome! Please read our [contributing guidelines](https://github.com/arduino/TriggerPatternRecognition/blob/main/CONTRIBUTING.md) before submitting a pull request.