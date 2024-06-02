#include "utils.h"
#include <LittleFS.h>
#include <esplog.h>
String composeClientID()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    String reducedMac;

    for (int i = 3; i < 6; ++i)
    {
        reducedMac += String(mac[i], 16);
    }
    String clientId = "doorman-";
    clientId += reducedMac;
    return clientId;
}

bool formatLittleFS()
{
    log_warn("need to format LittleFS: ");
    LittleFS.end();
    LittleFS.begin();
    log_info("Success: %d", LittleFS.format());
    return LittleFS.begin();
}
