#include <uri/UriBraces.h>
#include <esplog.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "apiserver.h"
#include "platform.h"
#include "html.h"

void ApiServer::begin()
{
    m_server.begin(80);

    // Webpages
    //m_server.on("/", HTTP_GET, std::bind(&ApiServer::handleRoot, this));
    //m_server.on(UriBraces("/{}"), HTTP_GET, std::bind(&ApiServer::handleWeb, this));

    m_server.on("/", HTTP_GET, std::bind(&ApiServer::handleMainPage, this));
    m_server.on("/codes", HTTP_GET, std::bind(&ApiServer::handleCodesPage, this));
    m_server.on("/settings", HTTP_GET, std::bind(&ApiServer::handleSettingsPage, this));

    // Register API endpoints
    m_server.on("/api/v1/config", HTTP_GET, std::bind(&ApiServer::handleDeviceConfig, this));
    m_server.on("/api/v1/codes", HTTP_GET, std::bind(&ApiServer::handleCodeConfig, this));
    m_server.on("/api/v1/codes", HTTP_POST, std::bind(&ApiServer::handleCodeConfigSave, this));
    m_server.on("/api/v1/device/settings", HTTP_GET, std::bind(&ApiServer::handleDeviceSettings, this));
    m_server.on("/api/v1/device/settings", HTTP_POST, std::bind(&ApiServer::handleDeviceSettingsSave, this));
    //m_server.on("/api/v1/device/wifi", HTTP_POST, std::bind(&ApiServer::handleDeviceWifiPost, this));
    //m_server.on("/api/v1/device/info", HTTP_GET, std::bind(&ApiServer::handleDeviceInfo, this));
    m_server.on("/api/v1/device/reboot", HTTP_GET, std::bind(&ApiServer::handleDeviceReboot, this));
    //m_server.on("/api/v1/runs/list", HTTP_GET, std::bind(&ApiServer::handleListRuns, this));
    //m_server.on(UriBraces("/api/v1/runs/data/{}"), HTTP_GET, std::bind(&ApiServer::handleRunData, this));
    //m_server.on(UriBraces("/api/v1/runs/bin/{}"), HTTP_GET, std::bind(&ApiServer::handleRunBinary, this));
    //m_server.on(UriBraces("/api/v1/runs/csv/{}"), HTTP_GET, std::bind(&ApiServer::handleRunCSV, this));
    //m_server.on(UriBraces("/api/v1/runs/delete/{}"), HTTP_GET, std::bind(&ApiServer::handleRunDelete, this));
}

// WEB PAGES

void ApiServer::handleMainPage()
{
    // TODO: Fix
    m_server.send_P(200, "text/html", PAGE_MAIN);
}

void ApiServer::handleCodesPage()
{
    // TODO: fix
    m_server.send_P(200, "text/html", PAGE_CODES);
}

void ApiServer::handleSettingsPage()
{
    m_server.send_P(200, "text/html", PAGE_SETTINGS);
}

void ApiServer::handleRoot()
{
    // Redirect to the main page URL
    m_server.sendHeader("Location", "/index.html", true);
    m_server.send(302, "text/plain", "");
}

void ApiServer::handleWeb()
{
    String fileName = m_server.pathArg(0);
    File file = LittleFS.open("/www/" + fileName, "r");
    if (!file)
    {
        m_server.send(404, "text/plain", "File not found");
        return;
    }
    String contentType = getContentType(fileName);
    m_server.streamFile(file, contentType);
    file.close();
}

void ApiServer::handleDeviceConfig()
{
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file)
    {
        m_server.send(404, "text/plain", "File not found");
        return;
    }
    m_server.streamFile(file, "application/json");
    file.close();
}

void ApiServer::handleDeviceSettings()
{
    // Respond with the current configuration in JSON format
    String configJson;
    JsonDocument doc;

    // Set the values in the document
    Settings::WiFiSettings wifiSettings = m_settings->getWiFiSettings();

    doc["wifiSsid"] = wifiSettings.staSsid;
    //doc["wifiPassword"] = wifiSettings.staPassword;

    doc["apSsid"] = wifiSettings.apSsid;
    doc["apPassword"] = wifiSettings.apPassword;

    Settings::MqttSettings mqttSettings = m_settings->getMqttSettings();
    doc["mqttServer"] = mqttSettings.mqttServer;

    doc["mqttPort"] = mqttSettings.mqttPort;
    doc["mqttUser"] = mqttSettings.mqttUser;
    doc["mqttPassword"] = mqttSettings.mqttPassword;
    doc["version"] = SYSTEM_NAME " " VERSION " (" __DATE__ ")";

    serializeJson(doc, configJson);
    m_server.send(200, "application/json", configJson);
}

void ApiServer::handleDeviceSettingsSave()
{
    // Handle form submission and update configuration data
    if (m_server.method() == HTTP_POST)
    {
        JsonDocument doc;
        String jsonString = m_server.arg("plain");
        deserializeJson(doc, jsonString);

        Settings::WiFiSettings wifiSettings = m_settings->getWiFiSettings();
        Settings::MqttSettings mqttSettings = m_settings->getMqttSettings();

        // Update the currentConfig struct with the new values from the form submission
        if (doc.containsKey("wifiSsid"))
        {
            strncpy(wifiSettings.staSsid, doc["wifiSsid"].as<const char *>(), sizeof(wifiSettings.staSsid));
        }
        // only save password if not empty
        if (doc.containsKey("wifiPassword") and strlen(doc["wifiPassword"]) > 0)
        {
            strncpy(wifiSettings.staPassword, doc["wifiPassword"].as<const char *>(), sizeof(wifiSettings.staPassword));
        }
        if (doc.containsKey("mqttServer"))
        {
            strncpy(mqttSettings.mqttServer, doc["mqttServer"].as<const char *>(), sizeof(mqttSettings.mqttServer));
        }
        mqttSettings.mqttPort = doc["mqttPort"] | mqttSettings.mqttPort;
        if (doc.containsKey("mqttUser"))
        {
            strncpy(mqttSettings.mqttUser, doc["mqttUser"].as<const char *>(), sizeof(mqttSettings.mqttUser));
        }
        if (doc.containsKey("mqttPassword"))
        {
            strncpy(mqttSettings.mqttPassword, doc["mqttPassword"].as<const char *>(), sizeof(mqttSettings.mqttPassword));
        }
        m_settings->setMqttSettings(mqttSettings);
        m_settings->setWiFiSettings(wifiSettings);
        m_settings->save();

        // TODO: force update of mqtt view
        // publishMqttConfigValues();

        // Send a response to the client
        String responseMessage = "Configuration updated successfully!";
        m_server.send(200, "application/json", "{\"message\":\"" + responseMessage + "\"}");
    }
}

void ApiServer::handleDeviceReboot()
{
    // wait a bit to save all files
    delay(100);
    m_server.send(200);
    // TODO: the restart should probably be
    // somewhere else to give a correct response
    delay(100);
    ESP.restart();
}

void ApiServer::handleCodeConfig()
{
    // Respond with the current configuration in JSON format
    String configJson;
    JsonDocument doc;
    Settings::CodeSettings codeSettings = m_settings->getCodeSettings();
    Settings::GeneralSettings generalSettings = m_settings->getGeneralSettings();
    // Set the values in the document
    doc["codeApartmentDoorBell"] = codeSettings.codeApartmentDoorBell;
    doc["codeEntryDoorBell"] = codeSettings.codeEntryDoorBell;
    doc["codeHandsetLiftup"] = codeSettings.codeHandsetLiftup;
    doc["codeDoorOpener"] = codeSettings.codeDoorOpener;
    doc["codeApartmentPatternDetect"] = codeSettings.codeApartmentPatternDetect;
    doc["codeEntryPatternDetect"] = codeSettings.codeEntryPatternDetect;
    doc["codePartyMode"] = codeSettings.codePartyMode;
    doc["restartCounter"] = generalSettings.restartCounter;
    doc["version"] = SYSTEM_NAME " " VERSION " (" __DATE__ ")";

    serializeJson(doc, configJson);
    m_server.send(200, "application/json", configJson);
}

void ApiServer::handleCodeConfigSave()
{
    // Handle form submission and update configuration data
    if (m_server.method() == HTTP_POST)
    {
        JsonDocument doc;
        String jsonString = m_server.arg("plain");
        deserializeJson(doc, jsonString);

        // Update the currentConfig struct with the new values from the form submission
        // Copy values from the JsonDocument to the Config

        Settings::CodeSettings codeSettings = m_settings->getCodeSettings();
    
        codeSettings.codeApartmentDoorBell = doc["codeApartmentDoorBell"] | codeSettings.codeApartmentDoorBell;
        codeSettings.codeEntryDoorBell = doc["codeEntryDoorBell"] | codeSettings.codeEntryDoorBell;
        codeSettings.codeHandsetLiftup = doc["codeHandsetLiftup"] | codeSettings.codeHandsetLiftup;
        codeSettings.codeDoorOpener = doc["codeDoorOpener"] | codeSettings.codeDoorOpener;
        codeSettings.codeApartmentPatternDetect = doc["codeApartmentPatternDetect"] | codeSettings.codeApartmentPatternDetect;
        codeSettings.codeEntryPatternDetect = doc["codeEntryPatternDetect"] | codeSettings.codeEntryPatternDetect;
        codeSettings.codePartyMode = doc["codePartyMode"] | codeSettings.codePartyMode;

        m_settings->setCodeSettings(codeSettings);
        m_settings->save();

        // TODO: force update of mqtt view! 
        //g_mqttView.publishConfigValues(g_config);

        // Send a response to the client
        String responseMessage = "Configuration updated successfully!";
        m_server.send(200, "application/json", "{\"message\":\"" + responseMessage + "\"}");
    }
}

void ApiServer::handleClient()
{
    m_server.handleClient();
}

// HELPERS

String ApiServer::getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".png"))
        return "image/png";
    else if (filename.endsWith(".jpg"))
        return "image/jpeg";
    else if (filename.endsWith(".gif"))
        return "image/gif";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else if (filename.endsWith(".xml"))
        return "text/xml";
    else if (filename.endsWith(".pdf"))
        return "application/x-pdf";
    else if (filename.endsWith(".zip"))
        return "application/x-zip";
    else if (filename.endsWith(".gz"))
        return "application/x-gzip";
    return "text/plain";
}
