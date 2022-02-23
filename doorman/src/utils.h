#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h> 

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