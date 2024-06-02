#pragma once
#include <Arduino.h>

#define VERSION "2024.6.0"

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

#ifdef ARDUINO_LOLIN_S3_MINI
#define SUPPORT_RGB_LED 1
#define RGB_LED_PIN 47
#endif

const char DEFAULT_AP_PASSWORD[] = "doormanadmin";