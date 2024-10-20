#pragma once
#include <Arduino.h>

#define VERSION "2024.4.1"

#ifdef ESP8266
#define PIN_BUS_READ D5
#define PIN_BUS_WRITE D6
#define SYSTEM_NAME "ESP8266 Doorman"
#endif

#ifdef DOORMAN_S3
#define PIN_BUS_READ 9
#define PIN_BUS_WRITE 8
#define SYSTEM_NAME "Doorman-S3"
#else
    #ifdef ESP32
    #define PIN_BUS_READ 12
    #define PIN_BUS_WRITE 13
    #define SYSTEM_NAME "ESP32 Doorman"
    #endif
#endif

