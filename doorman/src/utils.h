#pragma once
#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h> 
#endif


String macToStr(const uint8_t *mac)
{
    String result;
    for (int i = 3; i < 6; ++i)
    {
        result += String(mac[i], 16);
        //if (i < 5)
        //    result += ':';
    }
    return result;
}

String composeClientID()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    String clientId = "doorman-";
    clientId += macToStr(mac);
    return clientId;
}