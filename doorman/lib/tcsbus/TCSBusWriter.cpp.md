## Table of Contents 📖

- [Introduction](#introduction)
- [Class Overview](#class-overview)
- [Member Functions](#member-functions)
    - [Constructor](#constructor)
    - [begin()](#begin)
    - [isWriting()](#iswriting)
    - [write()](#write)
- [Example Usage](#example-usage)

## Introduction 💡

The `TCSBusWriter` class enables writing data to a TCS bus, which is a proprietary communication protocol commonly used in TCS intercom systems. It provides a high-level API to simplify the low-level management of the write pin and data transmission.

## Class Overview 💻

The `TCSBusWriter` class initializes the write pin, manages the writing state, and handles the precise timing and bit manipulation required for data transmission on the TCS bus.

## Member Functions 🛠️

### Constructor

#### `TCSBusWriter(uint8_t writePin)`

- **Input:** `writePin` (uint8_t) - The digital pin connected to the TCS bus write line.
- **Returns:** None
- **Functionality:**
  - Constructs a new `TCSBusWriter` object with the specified write pin.
  - Sets the write pin as an output.

### `begin()`

- **Input:** None
- **Returns:** None
- **Functionality:**
  - Initializes the write pin as an output.
  - Prepares the `TCSBusWriter` for data transmission.

### `isWriting()`

- **Input:** None
- **Returns:** bool - `true` if the `TCSBusWriter` is currently writing data, `false` otherwise.
- **Functionality:**
  - Checks if the `TCSBusWriter` is in the process of writing data to the TCS bus.

### `write(uint32_t data)` 📝

- **Input:** `data` (uint32_t) - The 32-bit data value to be written to the TCS bus.
- **Returns:** None
- **Functionality:**
  - Transmits the specified 32-bit `data` value onto the TCS bus.
  - Handles the necessary bit manipulation, timing, and checksum calculations.
  - The maximum message length is 32 bits.

## Example Usage 💻

```cpp
// Create a TCSBusWriter object for pin 10
TCSBusWriter writer(10);

// Initialize the writer
writer.begin();

// Write data to the TCS bus
writer.write(0x12345678);

// Check if the writer is currently writing
if (writer.isWriting()) {
  // Wait for the write operation to complete
}
```