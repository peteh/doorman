#pragma once

#include <Arduino.h>
struct Config
{
    uint32_t codeApartmentDoorBell;
    uint32_t codeEntryDoorBell;
    uint32_t codeHandsetLiftup;
    uint32_t codeDoorOpener;
    uint32_t codeApartmentPatternDetect;
    uint32_t codeEntryPatternDetect;
    uint32_t codePartyMode;
    uint32_t restartCounter;
    uint32_t wifiDisconnectCounter;
    uint32_t mqttDisconnectCounter;
    char wifiSsid[200];
    char wifiPassword[200];
    char mqttServer[200];
    uint16_t mqttPort;
    char mqttUser[200];
    char mqttPassword[200];
    bool partyMode;
    bool relayState;
};

#define CONFIG_FILENAME "/config.txt"