#include <esplog.h>
#include <LittleFS.h>
#include "platform.h"
#include "utils.h"
#include "settings.h"
#include "config.h"

#define CONFIG_KEY_WIFI_MODE "wifiMode"
#define CONFIG_KEY_WIFI_AP_SSID "wifiAPSsid"
#define CONFIG_KEY_WIFI_AP_PASSWORD "wifiAPPassword"

#define CONFIG_KEY_WIFI_STA_SSID "wifiSTASsid"
#define CONFIG_KEY_WIFI_STA_PASSWORD "wifiSTAPassword"

#define CONFIG_KEY_CODE_APARTMENT_DOOR_BELL "codeApartmentDoorBell"
#define CONFIG_KEY_CODE_ENTRY_DOOR_BELL "codeEntryDoorBell"
#define CONFIG_KEY_CODE_HANDSET_LIFTUP "codeHandsetLiftup"
#define CONFIG_KEY_CODE_ENTRY_DOOR_OPENER "codeDoorOpener"
#define CONFIG_KEY_CODE_APARTMENT_PATTERN_DETECT "codeApartmentPatternDetect"
#define CONFIG_KEY_CODE_ENTRY_PATTERN_DETECT "codeEntryPatternDetect"
#define CONFIG_KEY_CODE_PARTY_MODE "codePartyMode"

#define CONFIG_KEY_RESTART_COUNTER "restartCounter"
#define CONFIG_KEY_WIFI_DISCONNECT_COUNTER "wifiDisconnectCounter"
#define CONFIG_KEY_MQTT_DISCONNECT_COUNTER "mqttDisconnectCounter"
#define CONFIG_KEY_PARTY_MODE "partyMode"

#define CONFIG_KEY_MQTT_SERVER "mqttServer"
#define CONFIG_KEY_MQTT_PORT "mqttPort"
#define CONFIG_KEY_MQTT_USER "mqttUser"
#define CONFIG_KEY_MQTT_PASSWORD "mqttPassword"

void Settings::loadWifiSettings(JsonDocument &doc)
{
    // Default mode: access point with ssid doorman-[mac]
    m_wifiSettings.mode = WifiModeSetting::WIFIMODE_SETTING_AP;
    strncpy(m_wifiSettings.apSsid, composeClientID().c_str(), sizeof(m_wifiSettings.apSsid) - 1);
    strncpy(m_wifiSettings.apPassword, DEFAULT_AP_PASSWORD, sizeof(m_wifiSettings.apPassword) - 1);

    strncpy(m_wifiSettings.staSsid, DEFAULT_STA_WIFI_SSID, sizeof(m_wifiSettings.staSsid) - 1);
    strncpy(m_wifiSettings.staPassword, DEFAULT_STA_WIFI_PASS, sizeof(m_wifiSettings.staPassword) - 1);

    if (doc.containsKey(CONFIG_KEY_WIFI_STA_SSID))
    {
        strncpy(m_wifiSettings.staSsid, doc[CONFIG_KEY_WIFI_STA_SSID].as<const char *>(), sizeof(m_wifiSettings.staSsid) - 1);
    }

    if (doc.containsKey(CONFIG_KEY_WIFI_STA_PASSWORD))
    {
        strncpy(m_wifiSettings.staPassword, doc[CONFIG_KEY_WIFI_STA_PASSWORD].as<const char *>(), sizeof(m_wifiSettings.staPassword) - 1);
    }

    if (doc.containsKey(CONFIG_KEY_WIFI_AP_SSID))
    {
        strncpy(m_wifiSettings.apSsid, doc[CONFIG_KEY_WIFI_AP_SSID].as<const char *>(), sizeof(m_wifiSettings.apSsid) - 1);
    }

    if (doc.containsKey(CONFIG_KEY_WIFI_AP_PASSWORD))
    {
        strncpy(m_wifiSettings.apPassword, doc[CONFIG_KEY_WIFI_AP_PASSWORD].as<const char *>(), sizeof(m_wifiSettings.apPassword) - 1);
    }

    if (doc.containsKey(CONFIG_KEY_WIFI_MODE))
    {
        char buffer[20];
        strncpy(buffer, doc[CONFIG_KEY_WIFI_MODE].as<const char *>(), sizeof(buffer) - 1);

        // can be STA or AP
        if (strcmp("STA", buffer) == 0)
        {
            m_wifiSettings.mode = WifiModeSetting::WIFIMODE_SETTING_STA;
        }
        else if (strcmp("AP", buffer) == 0)
        {
            m_wifiSettings.mode = WifiModeSetting::WIFIMODE_SETTING_AP;
        }
        else
        {
            log_warn("Config loading: invalid WiFi Mode: %s, falling back to AP", buffer);
            m_wifiSettings.mode = WifiModeSetting::WIFIMODE_SETTING_AP;
        }
    }
}

void Settings::loadMqttSettings(JsonDocument &doc)
{
    if (doc.containsKey(CONFIG_KEY_MQTT_SERVER))
    {
        strncpy(m_mqttSettings.mqttServer, doc[CONFIG_KEY_MQTT_SERVER].as<const char *>(), sizeof(m_mqttSettings.mqttServer));
    }
    m_mqttSettings.mqttPort = doc[CONFIG_KEY_MQTT_PORT] | m_mqttSettings.mqttPort;
    if (doc.containsKey(CONFIG_KEY_MQTT_USER))
    {
        strncpy(m_mqttSettings.mqttUser, doc[CONFIG_KEY_MQTT_USER].as<const char *>(), sizeof(m_mqttSettings.mqttUser));
    }
    if (doc.containsKey(CONFIG_KEY_MQTT_PASSWORD))
    {
        strncpy(m_mqttSettings.mqttPassword, doc[CONFIG_KEY_MQTT_PASSWORD].as<const char *>(), sizeof(m_mqttSettings.mqttPassword));
    }
}

void Settings::loadCodeSettings(JsonDocument &doc)
{
    m_codeSettings.codeApartmentDoorBell = doc[CONFIG_KEY_CODE_APARTMENT_DOOR_BELL] | m_codeSettings.codeApartmentDoorBell;
    m_codeSettings.codeEntryDoorBell = doc[CONFIG_KEY_CODE_ENTRY_DOOR_BELL] | m_codeSettings.codeEntryDoorBell;
    m_codeSettings.codeHandsetLiftup = doc[CONFIG_KEY_CODE_HANDSET_LIFTUP] | m_codeSettings.codeHandsetLiftup;
    m_codeSettings.codeDoorOpener = doc[CONFIG_KEY_CODE_ENTRY_DOOR_OPENER] | m_codeSettings.codeDoorOpener;
    m_codeSettings.codeApartmentPatternDetect = doc[CONFIG_KEY_CODE_APARTMENT_PATTERN_DETECT] | m_codeSettings.codeApartmentPatternDetect;
    m_codeSettings.codeEntryPatternDetect = doc[CONFIG_KEY_CODE_ENTRY_PATTERN_DETECT] | m_codeSettings.codeEntryPatternDetect;
    m_codeSettings.codePartyMode = doc[CONFIG_KEY_CODE_PARTY_MODE] | m_codeSettings.codePartyMode;
}

void Settings::loadGeneralSettings(JsonDocument &doc)
{
    m_generalSettings.restartCounter = doc[CONFIG_KEY_RESTART_COUNTER] | m_generalSettings.restartCounter;
    m_generalSettings.wifiDisconnectCounter = doc[CONFIG_KEY_WIFI_DISCONNECT_COUNTER] | m_generalSettings.wifiDisconnectCounter;
    m_generalSettings.mqttDisconnectCounter = doc[CONFIG_KEY_MQTT_DISCONNECT_COUNTER] | m_generalSettings.mqttDisconnectCounter;
    m_generalSettings.partyMode = doc[CONFIG_KEY_PARTY_MODE] | m_generalSettings.partyMode;
}

void Settings::begin()
{
    // check if file exists, if not create it
    if (!LittleFS.exists(CONFIG_FILE))
    {
        JsonDocument doc;
        log_warn("Settings file does not exist, creating it...");
        File configFile = LittleFS.open(CONFIG_FILE, "w");
        if (serializeJson(doc, configFile) == 0)
        {
            log_error("Failed to create empty config file");
        }
        configFile.close();
    }

    // LOAD FROM CONFIG FILE
    File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile)
    {
        log_error("Cannot open config file %s, aborting", CONFIG_FILE);
        return;
    }

    // Parse the JSON data from the file
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);

    if (error)
    {
        Serial.println("Failed to parse config file");
        return;
    }

    loadWifiSettings(doc);
    loadMqttSettings(doc);
    loadCodeSettings(doc);
    loadGeneralSettings(doc);
}

void Settings::save()
{
    JsonDocument doc;

    saveWifiSettings(doc);
    saveMqttSettings(doc);
    saveCodeSettings(doc);
    saveGeneralSettings(doc);

    // Open the configuration file for writing
    File configFile = LittleFS.open(CONFIG_FILE, "w");

    if (configFile)
    {
        // Serialize the JSON document to the file
        if (serializeJson(doc, configFile) == 0)
        {
            log_error("Failed to write to config file");
        }

        // Close the file
        configFile.close();
    }
}

void Settings::saveWifiSettings(JsonDocument &doc)
{
    doc[CONFIG_KEY_WIFI_MODE] = (m_wifiSettings.mode == WifiModeSetting::WIFIMODE_SETTING_STA) ? "STA" : "AP";
    doc[CONFIG_KEY_WIFI_AP_SSID] = m_wifiSettings.apSsid;
    doc[CONFIG_KEY_WIFI_AP_PASSWORD] = m_wifiSettings.apPassword;
    doc[CONFIG_KEY_WIFI_STA_SSID] = m_wifiSettings.staSsid;
    doc[CONFIG_KEY_WIFI_STA_PASSWORD] = m_wifiSettings.staPassword;
}

void Settings::saveMqttSettings(JsonDocument &doc)
{
    doc[CONFIG_KEY_MQTT_SERVER] = m_mqttSettings.mqttServer;
    doc[CONFIG_KEY_MQTT_PORT] = m_mqttSettings.mqttPort;
    doc[CONFIG_KEY_MQTT_USER] = m_mqttSettings.mqttUser;
    doc[CONFIG_KEY_MQTT_PASSWORD] = m_mqttSettings.mqttPassword;
}

void Settings::saveCodeSettings(JsonDocument &doc)
{
    doc[CONFIG_KEY_CODE_APARTMENT_DOOR_BELL] = m_codeSettings.codeApartmentDoorBell;
    doc[CONFIG_KEY_CODE_ENTRY_DOOR_BELL] = m_codeSettings.codeEntryDoorBell;
    doc[CONFIG_KEY_CODE_HANDSET_LIFTUP] = m_codeSettings.codeHandsetLiftup;
    doc[CONFIG_KEY_CODE_ENTRY_DOOR_OPENER] = m_codeSettings.codeDoorOpener;
    doc[CONFIG_KEY_CODE_APARTMENT_PATTERN_DETECT] = m_codeSettings.codeApartmentPatternDetect;
    doc[CONFIG_KEY_CODE_ENTRY_PATTERN_DETECT] = m_codeSettings.codeEntryPatternDetect;
    doc[CONFIG_KEY_CODE_PARTY_MODE] = m_codeSettings.codePartyMode;
}

void Settings::saveGeneralSettings(JsonDocument &doc)
{
    doc[CONFIG_KEY_RESTART_COUNTER] = m_generalSettings.restartCounter;
    doc[CONFIG_KEY_WIFI_DISCONNECT_COUNTER] = m_generalSettings.wifiDisconnectCounter;
    doc[CONFIG_KEY_MQTT_DISCONNECT_COUNTER] = m_generalSettings.mqttDisconnectCounter;
    doc[CONFIG_KEY_PARTY_MODE] = m_generalSettings.partyMode;
}

const Settings::WiFiSettings Settings::getWiFiSettings() const
{
    return m_wifiSettings;
}

void Settings::setWiFiSettings(const WiFiSettings wifiSettings)
{
    m_wifiSettings = wifiSettings;
}

const Settings::CodeSettings Settings::getCodeSettings() const
{
    return m_codeSettings;
}

void Settings::setCodeSettings(const CodeSettings codeSettings)
{
    m_codeSettings = codeSettings;
}

const Settings::GeneralSettings Settings::getGeneralSettings() const
{
    return m_generalSettings;
}

void Settings::setGeneralSettings(const GeneralSettings generalSettings)
{
    m_generalSettings = generalSettings;
}

const Settings::MqttSettings Settings::getMqttSettings() const
{
    return m_mqttSettings;
}

void Settings::setMqttSettings(const MqttSettings mqttSettings)
{
    m_mqttSettings = mqttSettings;
}