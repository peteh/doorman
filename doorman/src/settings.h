#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#endif
#define CONFIG_FILE "/config.json"

class Settings
{
public:
    struct Config
    {

        uint32_t restartCounter;
        uint32_t wifiDisconnectCounter;
        uint32_t mqttDisconnectCounter;
        bool partyMode;
    };

    enum WifiModeSetting{
        WIFIMODE_SETTING_AP,
        WIFIMODE_SETTING_STA
    };

    struct WiFiSettings
    {
        WifiModeSetting mode;
        char staSsid[33];
        char staPassword[64];
        char apSsid[33];
        char apPassword[64];
    };

    struct MqttSettings
    {
        bool mqttActive;
        char mqttServer[200];
        uint16_t mqttPort;
        char mqttUser[200];
        char mqttPassword[200];
    };

    struct CodeSettings
    {
        uint32_t codeApartmentDoorBell;
        uint32_t codeEntryDoorBell;
        uint32_t codeHandsetLiftup;
        uint32_t codeDoorOpener;
        uint32_t codeApartmentPatternDetect;
        uint32_t codeEntryPatternDetect;
        uint32_t codePartyMode;
    };

    struct GeneralSettings
    {
        uint32_t restartCounter;
        uint32_t wifiDisconnectCounter;
        uint32_t mqttDisconnectCounter;
        bool partyMode;
    };

private:
    WiFiSettings m_wifiSettings{
        WifiModeSetting::WIFIMODE_SETTING_AP,
        "",
        "",
        "",
        "",
    };

    MqttSettings m_mqttSettings{
        false,
        "",
        1883,
        "",
        ""};

    CodeSettings m_codeSettings{
        0,
        0,
        0,
        0,
        0,
        0,
        0};

    GeneralSettings m_generalSettings{
        0,
        0,
        0,
        false};

private:
    void initWifiSettings();

    void loadWifiSettings(JsonDocument &doc);
    void loadMqttSettings(JsonDocument &doc);
    void loadCodeSettings(JsonDocument &doc);
    void loadGeneralSettings(JsonDocument &doc);

    void saveWifiSettings(JsonDocument &doc);
    void saveMqttSettings(JsonDocument &doc);
    void saveCodeSettings(JsonDocument &doc);
    void saveGeneralSettings(JsonDocument &doc);

public:
    void begin();

    const CodeSettings getCodeSettings() const;
    void setCodeSettings(const CodeSettings codeSettings);

    const GeneralSettings getGeneralSettings() const;
    void setGeneralSettings(const GeneralSettings codeSettings);

    const WiFiSettings getWiFiSettings() const;
    void setWiFiSettings(const WiFiSettings wifiSettings);

    const MqttSettings getMqttSettings() const;
    void setMqttSettings(const MqttSettings mqttSettings);

    void save();
};
