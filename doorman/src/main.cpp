#include <Arduino.h>
#include <DNSServer.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif

#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
#include <mdns.h>
// watch dog
#include <esp_task_wdt.h>
#endif

#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <esplog.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>

#include <TCSBus.h>
#include <TriggerPatternRecognition.h>

#include <MqttDevice.h>

#include "datastruct.h"
#include "platform.h"
#include "configstorage.h"
#include "utils.h"
#include "config.h"
#include "html.h"
#include "mqttview.h"

const uint WATCHDOG_TIMEOUT_S = 30;
const uint WIFI_DISCONNECT_FORCED_RESTART_S = 60;

WiFiClient net;
PubSubClient client(net);
MqttView g_mqttView(&client);

#ifdef ESP8266
ESP8266WebServer server(80);
#endif

#ifdef ESP32
WebServer server(80);
#endif

#ifdef ARDUINO_LOLIN_S3_MINI
#define SUPPORT_RGB_LED 1
#define RGB_LED_PIN 47
#endif

#ifdef SUPPORT_RGB_LED
#include "led_rgb.h"
Led *g_led = new LedRGB(RGB_LED_PIN);
#else
#include "led_builtin.h"
Led *g_led = new LedBuiltin(LED_BUILTIN);
#endif

const char *HOMEASSISTANT_STATUS_TOPIC = "homeassistant/status";
const char *HOMEASSISTANT_STATUS_TOPIC_ALT = "ha/status";

Config g_config = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", "", 1883, false};

TriggerPatternRecognition patternRecognitionEntry;
TriggerPatternRecognition patternRecognitionApartment;
TCSBusWriter tcsWriter(PIN_BUS_WRITE);
TCSBusReader tcsReader(PIN_BUS_READ);

uint32_t g_commandToSend = 0;
bool g_shouldSend = false;

bool g_ledState = false;
unsigned long g_tsLastLedStateOn = 0;

uint8_t g_handsetLiftup = 0;
unsigned long g_tsLastHandsetLiftup = 0;

bool g_wifiConnected = false;
bool g_mqttConnected = false;
unsigned long g_lastWifiConnect = 0;

String g_bssid = "";
// TODO: wifi auto config
// TODO: publish persistant

bool connectToMqtt()
{
    if (client.connected())
    {
        return true;
    }

    log_info("Connecting to MQTT...");
    if (strlen(mqtt_user) == 0)
    {
        if (!client.connect(composeClientID().c_str()))
        {
            return false;
        }
    }
    else
    {
        if (!client.connect(composeClientID().c_str(), mqtt_user, mqtt_pass))
        {
            return false;
        }
    }

    client.subscribe(g_mqttView.getApartmentBell().getCommandTopic(), 1);

    client.subscribe(g_mqttView.getEntryBell().getCommandTopic(), 1);
    client.subscribe(g_mqttView.getEntryOpener().getCommandTopic(), 1);

    client.subscribe(g_mqttView.getPartyMode().getCommandTopic(), 1);

    client.subscribe(g_mqttView.getBus().getCommandTopic(), 1);

    client.subscribe(g_mqttView.getConfigCodeApartmentDoorBell().getCommandTopic(), 1);
    client.subscribe(g_mqttView.getConfigCodeEntryDoorBell().getCommandTopic(), 1);
    client.subscribe(g_mqttView.getConfigCodeHandsetLiftup().getCommandTopic(), 1);
    client.subscribe(g_mqttView.getConfigCodeDoorOpener().getCommandTopic(), 1);
    client.subscribe(g_mqttView.getConfigCodeApartmentPatternDetect().getCommandTopic(), 1);
    client.subscribe(g_mqttView.getConfigCodeEntryPatternDetect().getCommandTopic(), 1);
    client.subscribe(g_mqttView.getConfigCodePartyMode().getCommandTopic(), 1);

    client.subscribe(g_mqttView.getDiagnosticsResetButton().getCommandTopic(), 1);

    client.subscribe(HOMEASSISTANT_STATUS_TOPIC);
    client.subscribe(HOMEASSISTANT_STATUS_TOPIC_ALT);

    // TODO: solve this somehow with auto discovery lib
    // client.publish(mqttTopic(MQTT_TOPIC_NONE, MQTT_ACTION_NONE).c_str(), "online");
    g_mqttView.publishConfig(g_config);

    return true;
}

bool connectToWifi()
{
    return WiFi.status() == WL_CONNECTED;
}

void printSettings()
{
    log_info("Code Apartment Door Bell: %08x", g_config.codeApartmentDoorBell);
    log_info("Code Entry Door Bell: %08x", g_config.codeEntryDoorBell);
    log_info("Code Handset Liftup: %08x", g_config.codeHandsetLiftup);
    log_info("Code Door Opener: %08x", g_config.codeDoorOpener);
    log_info("Code Apartment Door Pattern Detection: %08x", g_config.codeApartmentPatternDetect);
    log_info("Code Entry Door Pattern Detection: %08x", g_config.codeEntryPatternDetect);
    log_info("Code Party Mode: %08x", g_config.codePartyMode);

    log_info("Wifi SSID: %s", g_config.wifiSsid);
    log_info("Wifi Password: %s", g_config.wifiPassword);
    log_info("MQTT Server: %s", g_config.mqttServer);
    log_info("MQTT Port: %d", g_config.mqttPort);
    log_info("MQTT User: %s", g_config.mqttUser);
    log_info("MQTT Password: %s", g_config.mqttPassword);
}

void handleCodeConfig()
{
    // Respond with the current configuration in JSON format
    String configJson;
    StaticJsonDocument<1024> doc;

    // Set the values in the document
    doc["codeApartmentDoorBell"] = g_config.codeApartmentDoorBell;
    doc["codeEntryDoorBell"] = g_config.codeEntryDoorBell;
    doc["codeHandsetLiftup"] = g_config.codeHandsetLiftup;
    doc["codeDoorOpener"] = g_config.codeDoorOpener;
    doc["codeApartmentPatternDetect"] = g_config.codeApartmentPatternDetect;
    doc["codeEntryPatternDetect"] = g_config.codeEntryPatternDetect;
    doc["codePartyMode"] = g_config.codePartyMode;
    doc["restartCounter"] = g_config.restartCounter;
    doc["version"] = SYSTEM_NAME " " VERSION " (" __DATE__ ")";

    serializeJson(doc, configJson);
    server.send(200, "application/json", configJson);
}

void handleSettingsConfig()
{
    // Respond with the current configuration in JSON format
    String configJson;
    StaticJsonDocument<1024> doc;

    // Set the values in the document
    doc["wifiSsid"] = g_config.wifiSsid;
    // doc["wifiPassword"] = g_config.wifiPassword;
    doc["mqttServer"] = g_config.mqttServer;
    doc["mqttPort"] = g_config.mqttPort;
    doc["mqttUser"] = g_config.mqttUser;
    doc["mqttPassword"] = g_config.mqttPassword;
    doc["version"] = SYSTEM_NAME " " VERSION " (" __DATE__ ")";

    serializeJson(doc, configJson);
    server.send(200, "application/json", configJson);
}

void handleSaveSettingsConfig()
{
    // Handle form submission and update configuration data
    if (server.method() == HTTP_POST)
    {
        StaticJsonDocument<1024> doc;
        String jsonString = server.arg("plain");
        deserializeJson(doc, jsonString);

        // Update the currentConfig struct with the new values from the form submission
        if (doc.containsKey("wifiSsid"))
        {
            strncpy(g_config.wifiSsid, doc["wifiSsid"].as<const char *>(), sizeof(g_config.wifiSsid));
        }
        // only save password if not empty
        if (doc.containsKey("wifiPassword") and strlen(doc["wifiPassword"]) > 0)
        {
            strncpy(g_config.wifiPassword, doc["wifiPassword"].as<const char *>(), sizeof(g_config.wifiPassword));
        }
        if (doc.containsKey("mqttServer"))
        {
            strncpy(g_config.mqttServer, doc["mqttServer"].as<const char *>(), sizeof(g_config.mqttServer));
        }
        g_config.mqttPort = doc["mqttPort"] | g_config.mqttPort;
        if (doc.containsKey("mqttUser"))
        {
            strncpy(g_config.mqttUser, doc["mqttUser"].as<const char *>(), sizeof(g_config.mqttUser));
        }
        if (doc.containsKey("mqttPassword"))
        {
            strncpy(g_config.mqttPassword, doc["mqttPassword"].as<const char *>(), sizeof(g_config.mqttPassword));
        }
        saveSettings(g_config);
        // publishMqttConfigValues();

        // Send a response to the client
        String responseMessage = "Configuration updated successfully!";
        server.send(200, "application/json", "{\"message\":\"" + responseMessage + "\"}");
        printSettings();
    }
}

void handleSaveCodeConfig()
{
    // Handle form submission and update configuration data
    if (server.method() == HTTP_POST)
    {
        StaticJsonDocument<1024> doc;
        String jsonString = server.arg("plain");
        deserializeJson(doc, jsonString);

        // Update the currentConfig struct with the new values from the form submission
        // Copy values from the JsonDocument to the Config
        g_config.codeApartmentDoorBell = doc["codeApartmentDoorBell"] | g_config.codeApartmentDoorBell;
        g_config.codeEntryDoorBell = doc["codeEntryDoorBell"] | g_config.codeEntryDoorBell;
        g_config.codeHandsetLiftup = doc["codeHandsetLiftup"] | g_config.codeHandsetLiftup;
        g_config.codeDoorOpener = doc["codeDoorOpener"] | g_config.codeDoorOpener;
        g_config.codeApartmentPatternDetect = doc["codeApartmentPatternDetect"] | g_config.codeApartmentPatternDetect;
        g_config.codeEntryPatternDetect = doc["codeEntryPatternDetect"] | g_config.codeEntryPatternDetect;
        g_config.codePartyMode = doc["codePartyMode"] | g_config.codePartyMode;
        saveSettings(g_config);
        g_mqttView.publishConfigValues(g_config);

        // Send a response to the client
        String responseMessage = "Configuration updated successfully!";
        server.send(200, "application/json", "{\"message\":\"" + responseMessage + "\"}");
    }
}

void handleMainPage()
{
    server.send_P(200, "text/html", PAGE_MAIN);
}

void handleCodesPage()
{
    server.send_P(200, "text/html", PAGE_CODES);
}

void handleSettingsPage()
{
    server.send_P(200, "text/html", PAGE_SETTINGS);
}

void openDoor()
{
    delay(50);
    tcsWriter.write(g_config.codeDoorOpener);
}

uint32_t parseValue(const char *data, unsigned int length)
{
    // TODO length check
    char temp[32];
    strncpy(temp, data, length);
    return (uint32_t)strtoul(temp, NULL, 16);
}

void callback(char *topic, byte *payload, unsigned int length)
{
    log_info("Message arrived [%s]", topic);
    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (strcmp(topic, g_mqttView.getBus().getCommandTopic()) == 0)
    {
        uint32_t data = parseValue((char *)payload, length);
        g_commandToSend = data;
        g_shouldSend = true;
    }
    else if (strcmp(topic, g_mqttView.getEntryBell().getCommandTopic()) == 0)
    {
        g_commandToSend = g_config.codeEntryDoorBell;
        g_shouldSend = true;
    }
    else if (strcmp(topic, g_mqttView.getApartmentBell().getCommandTopic()) == 0)
    {
        g_commandToSend = g_config.codeApartmentDoorBell;
        g_shouldSend = true;
    }
    else if (strcmp(topic, g_mqttView.getEntryOpener().getCommandTopic()) == 0)
    {
        g_commandToSend = g_config.codeDoorOpener;
        g_shouldSend = true;
    }

    else if (strcmp(topic, g_mqttView.getPartyMode().getCommandTopic()) == 0)
    {
        g_config.partyMode = strncmp((char *)payload, g_mqttView.getPartyMode().getOnState(), length) == 0;
        g_led->setBackgroundLight(g_config.partyMode);
        g_led->blinkAsync(); // force update of led
        g_mqttView.publishPartyMode(g_config.partyMode);
    }

    // Save config entries from Homeassistant
    else if (strcmp(topic, g_mqttView.getConfigCodeApartmentDoorBell().getCommandTopic()) == 0)
    {
        g_config.codeApartmentDoorBell = parseValue((char *)payload, length);
        saveSettings(g_config);
        g_mqttView.publishConfigValues(g_config);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeEntryDoorBell().getCommandTopic()) == 0)
    {
        g_config.codeEntryDoorBell = parseValue((char *)payload, length);
        saveSettings(g_config);
        g_mqttView.publishConfigValues(g_config);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeHandsetLiftup().getCommandTopic()) == 0)
    {
        g_config.codeHandsetLiftup = parseValue((char *)payload, length);
        saveSettings(g_config);
        g_mqttView.publishConfigValues(g_config);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeDoorOpener().getCommandTopic()) == 0)
    {
        g_config.codeDoorOpener = parseValue((char *)payload, length);
        saveSettings(g_config);
        g_mqttView.publishConfigValues(g_config);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeApartmentPatternDetect().getCommandTopic()) == 0)
    {
        g_config.codeApartmentPatternDetect = parseValue((char *)payload, length);
        saveSettings(g_config);
        g_mqttView.publishConfigValues(g_config);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeEntryPatternDetect().getCommandTopic()) == 0)
    {
        g_config.codeEntryPatternDetect = parseValue((char *)payload, length);
        saveSettings(g_config);
        g_mqttView.publishConfigValues(g_config);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodePartyMode().getCommandTopic()) == 0)
    {
        g_config.codePartyMode = parseValue((char *)payload, length);
        saveSettings(g_config);
        g_mqttView.publishConfigValues(g_config);
    }

    else if (strcmp(topic, g_mqttView.getDiagnosticsResetButton().getCommandTopic()) == 0)
    {
        bool pressed = strncmp((char *)payload, g_mqttView.getDiagnosticsResetButton().getPressState(), length) == 0;
        g_config.restartCounter = 0;
        g_config.mqttDisconnectCounter = 0;
        g_config.wifiDisconnectCounter = 0;
        saveSettings(g_config);
        g_mqttView.publishDiagnostics(g_config, g_bssid.c_str());
    }

    // publish config when homeassistant comes online and needs the configuration again
    else if (strcmp(topic, HOMEASSISTANT_STATUS_TOPIC) == 0 ||
             strcmp(topic, HOMEASSISTANT_STATUS_TOPIC_ALT) == 0)
    {
        if (strncmp((char *)payload, "online", length) == 0)
        {
            g_mqttView.publishConfig(g_config);
        }
    }
}

void setup()
{
#ifdef ESP32
    // initialize watchdog
    esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true); // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);                      // add current thread to WDT watch
#endif

    g_led->begin();
    // turn on led until boot sequence finished
    g_led->blinkAsync();

    Serial.begin(115200);

    if (!LittleFS.begin())
    {
        log_error("Failed to mount file system");
        delay(5000);
        if (!formatLittleFS())
        {
            log_error("Failed to format file system - hardware issues!");
            for (;;)
            {
                delay(100);
            }
        }
    }
    loadSettings(g_config);
    g_config.restartCounter++;
    saveSettings(g_config);

    tcsWriter.begin();
    tcsReader.begin();

    // configure pattern detection
    patternRecognitionEntry.addStep(1000);
    patternRecognitionEntry.addStep(1000);

    // configure pattern detection
    patternRecognitionApartment.addStep(1000);
    patternRecognitionApartment.addStep(1000);

    WiFi.setHostname(composeClientID().c_str());
    WiFi.mode(WIFI_STA);
    #ifdef ESP32
        // select the AP with the strongest signal
        WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
        WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    #endif

    WiFi.begin(wifi_ssid, wifi_pass);


    log_info("Connecting to wifi...");
    // TODO: really forever? What if we want to go back to autoconnect?
    while (!connectToWifi())
    {
        log_debug(".");
        delay(500);
    }
    g_wifiConnected = true;
    g_lastWifiConnect = millis();

    log_info("Wifi connected!");

    log_info("Connected to SSID: %s", wifi_ssid);
    log_info("IP address: %s", WiFi.localIP().toString().c_str());
    g_bssid = WiFi.BSSIDstr();

    char configUrl[256];
    snprintf(configUrl, sizeof(configUrl), "http://%s/", WiFi.localIP().toString().c_str());
    g_mqttView.getDevice().setConfigurationUrl(configUrl);

    client.setBufferSize(1024);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    server.on("/", handleMainPage);
    server.on("/codes", handleCodesPage);
    server.on("/settings", handleSettingsPage);
    server.on("/setCodes", handleSaveCodeConfig);
    server.on("/getCodes", handleCodeConfig);
    server.on("/getSettings", handleSettingsConfig);
    server.on("/setSettings", handleSaveSettingsConfig);
    server.begin(); // Start the server
    log_info("Server listening");

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]()
                       {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    log_info("Start updating %s", type.c_str()); });
    ArduinoOTA.onEnd([]()
                     { log_info("End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          {
        // reset watchdog during update
        #ifdef ESP32
            esp_task_wdt_reset();
        #endif
        log_info("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
    log_error("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      log_error("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      log_error("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      log_error("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      log_error("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      log_error("End Failed");
    } });
    ArduinoOTA.begin();
}

void loop()
{
#ifdef ESP32
    // reset watchdog, important to be called once each loop.
    esp_task_wdt_reset();
#endif

    g_led->update();
    bool wifiConnected = connectToWifi();
    if (!wifiConnected)
    {
        if (g_wifiConnected)
        {
            // we switched to disconnected
            g_config.wifiDisconnectCounter++;
            saveSettings(g_config);
        }
        if (millis() - g_lastWifiConnect > WIFI_DISCONNECT_FORCED_RESTART_S * 1000)
        {
            log_warn("Wifi could not connect in time, will force a restart");
            esp_restart();
        }
        g_wifiConnected = false;
        g_mqttConnected = false;
        delay(1000);
        return;
    }
    g_wifiConnected = true;
    g_lastWifiConnect = millis();

    server.handleClient(); // Handling of incoming web requests
    ArduinoOTA.handle();

    bool mqttConnected = connectToMqtt();
    if (!mqttConnected)
    {
        if (g_mqttConnected)
        {
            // we switched to disconnected
            g_config.mqttDisconnectCounter++;
            saveSettings(g_config);
        }
        g_mqttConnected = false;
        delay(1000);
        return;
    }
    if (!g_mqttConnected)
    {
        // now we are successfully reconnected and publish our counters
        g_bssid = WiFi.BSSIDstr();
        g_mqttView.publishDiagnostics(g_config, g_bssid.c_str());
    }
    g_mqttConnected = true;

    client.loop();
    if (g_shouldSend)
    {
        uint32_t cmd = g_commandToSend;
        g_shouldSend = false;
        log_info("Sending: %08x", cmd);
        tcsReader.disable();
        tcsWriter.write(cmd);
        tcsReader.enable();
        // dirty hack to also publish commands we have written
        tcsReader.inject(cmd);
        g_led->blinkAsync();
    }
    if (tcsReader.hasCommand())
    {
        g_led->blinkAsync();
        uint32_t cmd = tcsReader.read();

        if (cmd == g_config.codeApartmentDoorBell)
        {
            g_mqttView.publishApartmentBellTrigger();
        }

        if (cmd == g_config.codeEntryDoorBell)
        {
            g_mqttView.publishEntryBellTrigger();
        }

        if (cmd == g_config.codeDoorOpener)
        {
            g_mqttView.publishEntryOpenerTrigger();
        }

        if (g_config.partyMode && cmd == g_config.codePartyMode)
        {
            // we have a party, let everybody in
            openDoor();
        }

        if (cmd == g_config.codeHandsetLiftup)
        {
            if (millis() - g_tsLastHandsetLiftup < 2000)
            {
                g_handsetLiftup++;
            }
            else
            {
                g_handsetLiftup = 1;
            }
            g_tsLastHandsetLiftup = millis();

            if (g_handsetLiftup == 3)
            {
                g_config.partyMode = !g_config.partyMode;
                g_handsetLiftup = 0;
                g_led->setBackgroundLight(g_config.partyMode);
                g_led->blinkAsync();
                g_mqttView.publishPartyMode(g_config.partyMode);
            }
        }

        if (cmd == g_config.codeEntryPatternDetect)
        {
            if (patternRecognitionEntry.trigger())
            {
                // TODO: make pattern configurable and enableable
                openDoor();
                g_mqttView.publishEntryBellPatternTrigger();
            }
        }

        if (cmd == g_config.codeApartmentPatternDetect)
        {
            if (patternRecognitionApartment.trigger())
            {
                g_mqttView.publishApartmentBellPatternTrigger();
            }
        }

        log_info("TCS Bus: %08x", cmd);
        g_mqttView.publishBus(cmd);
    }
}
