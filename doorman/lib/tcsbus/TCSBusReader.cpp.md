## TCSBusReader Class 📖

This class is designed to handle the decoding of commands received over the TCS Bus. It is implemented using an interrupt-driven approach to achieve high efficiency and real-time responsiveness.

### 📝 Table of Contents

- [Introduction](#introduction)
- [Constructor](#constructor)
- [Methods](#methods)
    - [begin()](#begin)
    - [enable()](#enable)
    - [disable()](#disable)
    - [hasCommand()](#hasCommand)
    - [read()](#read)
    - [inject()](#inject)
- [Interrupt Handler](#interrupt-handler)
- [Example Usage](#example-usage)

### 🌐 Introduction

The TCS Bus is a proprietary protocol used to communicate between TCS devices. It is a simple, yet effective protocol that allows for the reliable transmission of short commands between devices. This class provides a cross-platform implementation of a TCS Bus reader that allows for easy integration with various microcontrollers and embedded systems.

### 🛠️ Constructor

The TCSBusReader class constructor takes a single parameter:

- `readPin`: This parameter specifies the digital input pin that is used to read the TCS Bus signal.

```cpp
TCSBusReader(uint8_t readPin);
```

### 📚 Methods

#### [begin()](#begin)

The `begin()` method initializes the TCS Bus reader. It sets the specified `readPin` as an input and enables the interrupt-driven reading mechanism.

```cpp
void begin();
```

#### [enable()](#enable)

The `enable()` method enables the interrupt-driven reading mechanism. It must be called after `begin()` to start receiving and decoding commands.

```cpp
void enable();
```

#### [disable()](#disable)

The `disable()` method disables the interrupt-driven reading mechanism. It stops the reception and decoding of commands.

```cpp
void disable();
```

#### [hasCommand()](#hasCommand)

The `hasCommand()` method checks if a new command has been received and decoded. It returns `true` if a command is available and `false` otherwise.

```cpp
bool hasCommand();
```

#### [read()](#read)

The `read()` method reads the next decoded command from the TCS Bus. If no command is available, it returns `0`.

```cpp
uint32_t read();
```

#### [inject()](#inject)

The `inject()` method is used for testing purposes. It allows you to inject a pre-decoded command into the internal buffer, simulating its reception over the TCS Bus.

```cpp
void inject(uint32_t cmd);
```

### 💻 Interrupt Handler

The TCS Bus reader uses an interrupt-driven approach to analyze incoming signals. The interrupt handler function, named `analyzeCMD()`, is responsible for decoding the received commands in real-time. It employs a sophisticated algorithm to extract the command data and CRC from the incoming bitstream.

### 🧠 Example Usage

```cpp
#include "TCSBus.h"

const uint8_t TCS_BUS_READ_PIN = 7;

TCSBusReader reader(TCS_BUS_READ_PIN);

void setup() {
  reader.begin();
}

void loop() {
  if (reader.hasCommand()) {
    uint32_t cmd = reader.read();
    // Process the received command here...
  }
}
```

This example shows how to use the TCSBusReader class to receive and process commands over the TCS Bus.