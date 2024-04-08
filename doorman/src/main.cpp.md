## Documentation for `esp32-doorbell`

### Table of Contents
- [Overview](#Overview)
- [Features](#Features)
- [Usage](#Usage)
  - [Minimal Example](#Minimal-Example)
  - [Full Example](#Full-Example)
- [Configuration](#Configuration)
  - [MQTT Settings](#MQTT-Settings)
  - [WiFi Settings](#WiFi-Settings)
  - [Configuration API](#Configuration-API)
  - [Code Configuration](#Code-Configuration)
- [Hardware Connections](#Hardware-Connections)
- [Development Setup](#Development-Setup)

---

### Overview

`esp32-doorbell` is a firmware for ESP32 microcontrollers that turns them into a smart doorbell with the following features:

- 🏠 Supports both wired and wireless doorbells (using TCS and MQTT protocols)
- 📲 Mobile app and web interface for configuration and control
- 📊 Real-time monitoring of doorbell events
- 🎛️ Remote control of door opener
- 🚨 Party mode to automatically open the door for a limited time

---

### Features

- 🚪 **Doorbell detection**: Detects doorbell presses from both wired and wireless doorbells.
- 📱 **Mobile app and web interface**: Provides a convenient way to configure the doorbell, view doorbell events, and control the door opener remotely.
- 📊 **Real-time monitoring**: Monitors doorbell events in real-time, providing a comprehensive overview of doorbell activity.
- 🎛️ **Remote door opener control**: Allows users to open the door remotely using the mobile app or web interface.
- 🚨 **Party mode**: Automatically opens the door for a limited time, ideal for parties or when you're expecting multiple guests.

---

### Usage

#### Minimal Example

The following code snippet provides a minimal example of how to use `esp32-doorbell`:

```cpp
#include "esp32-doorbell.h"

void setup() {
  // Initialize the doorbell
  esp32Doorbell.begin();
}

void loop() {
  // Handle doorbell events
  esp32Doorbell.handleEvents();
}
```

#### Full Example

The following code snippet provides a more complete example of how to use `esp32-doorbell`:

```cpp
#include "esp32-doorbell.h"

void setup() {
  // Initialize the doorbell
  esp32Doorbell.begin();

  // Set up the WiFi connection
  esp32Doorbell.setWifiCredentials("my-ssid", "my-password");

  // Set up the MQTT connection
  esp32Doorbell.setMqttCredentials("my-mqtt-server", "my-mqtt-port", "my-mqtt-username", "my-mqtt-password");

  // Set up the door opener
  esp32Doorbell.setDoorOpenerPin(GPIO_NUM_25);
}

void loop() {
  // Handle doorbell events
  esp32Doorbell.handleEvents();

  // Check if the doorbell button was pressed
  if (esp32Doorbell.isDoorbellPressed()) {
    // Open the door
    esp32Doorbell.openDoor();
  }
}
```

---

### Configuration

#### MQTT Settings

| Setting | Description |
|---|---|
| MQTT Server | The address of the MQTT server |
| MQTT Port | The port of the MQTT server |
| MQTT Username | The username to use when connecting to the MQTT server |
| MQTT Password | The password to use when connecting to the MQTT server |

#### WiFi Settings

| Setting | Description |
|---|---|
| WiFi SSID | The name of the WiFi network to connect to |
| WiFi Password | The password of the WiFi network to connect to |

#### Configuration API

The `esp32-doorbell` firmware provides a REST API for configuring the doorbell. The following table lists the available API endpoints:

| Endpoint | Description |
|---|---|
| `/config` | Get the current configuration of the doorbell |
| `/config/set` | Set the configuration of the doorbell |

#### Code Configuration

The `esp32-doorbell` firmware allows you to configure the codes that are used to trigger different actions. The following table lists the available codes:

| Code | Description |
|---|---|
| Doorbell Code | The code that is used to trigger the doorbell |
| Door Opener Code | The code that is used to open the door |

---

### Hardware Connections

The following table shows the hardware connections for the `esp32-doorbell` firmware:

| Pin | Function |
|---|---|
| GPIO 0 | Doorbell input |
| GPIO 2 | Door opener output |
| GPIO 4 | LED output |

---

### Development Setup

To set up your development environment for `esp32-doorbell`, you will need the following:

- An ESP32 development board
- A USB cable
- The Arduino IDE
- The `esp32-doorbell` firmware

Once you have these, you can follow these steps to set up your development environment:

1. Install the Arduino IDE.
2. Connect your ESP32 development board to your computer using the USB cable.
3. Open the Arduino IDE.
4. Click on the "File" menu and select "Preferences".
5. In the "Preferences" window, click on the "Additional Boards Manager URLs" field and add the following URL: `https://dl.espressif.com/dl/package_esp32_index.json`.
6. Click on the "OK" button to close the "Preferences" window.
7. Click on the "Tools" menu and select "Board: " and then select your ESP32 development board from the list.
8. Click on the "Tools" menu and select "Port: " and then select the port that your ESP32 development board is connected to from the list.
9. Click on the "File" menu and select "Open".
10. Navigate to the directory where you downloaded the `esp32-doorbell` firmware.
11. Select the `esp32-doorbell.ino` file and click on the "Open" button.
12. Click on the "Upload" button to upload the firmware to your ESP32 development board.