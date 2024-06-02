#pragma once
#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h> 
#endif

String composeClientID();

bool formatLittleFS();
