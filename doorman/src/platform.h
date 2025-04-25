#pragma once
#include <Arduino.h>

#define VERSION "2025.04.0"

#ifdef ESP8266
    #define PIN_BUS_READ D5
    #define PIN_BUS_WRITE D6
    #define PIN_RELAY D7
    #define SYSTEM_NAME "ESP8266 Doorman"
#endif

#ifdef ARDUINO_LOLIN_S3_MINI
    #define SUPPORT_RGB_LED 1
    #define RGB_LED_PIN 47
    #define PIN_BUS_READ 12
    #define PIN_BUS_WRITE 13
    #define PIN_RELAY 14
#endif

#ifdef DOORMAN_S3
    #define SUPPORT_RGB_LED 1
    #define RGB_LED_PIN 2
    #define PIN_BUS_READ 9
    #define PIN_BUS_WRITE 8
    #define PIN_RELAY 42
    #define SYSTEM_NAME "Doorman-S3"
#else
    // generic ESP32
    #ifdef ESP32
        #define PIN_BUS_READ 12
        #define PIN_BUS_WRITE 13
        #define PIN_RELAY 14
        #define SYSTEM_NAME "ESP32 Doorman"
    #endif
#endif

const char DEFAULT_AP_PASSWORD[] = "doormanadmin";
const long WIFI_CONNECTION_FAIL_TIMEOUT_S = 60;