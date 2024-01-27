#pragma once
#include <Arduino.h>

#define VERSION "2024.1.1"

#ifdef ESP8266
#define PIN_BUS_READ D5
#define PIN_BUS_WRITE D6
#define SYSTEM_NAME "ESP8266 Doorman"
#endif

#ifdef ESP32
#define PIN_BUS_READ 12
#define PIN_BUS_WRITE 13
#define SYSTEM_NAME "ESP32 Doorman"
#endif