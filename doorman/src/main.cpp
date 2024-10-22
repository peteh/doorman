#include <Arduino.h>
#include <DNSServer.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#ifdef ESP32
#include <WiFi.h>
#include <mdns.h>
// watch dog
#include <esp_task_wdt.h>
#endif

//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <esplog.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>

#include <TCSBus.h>
#include <TriggerPatternRecognition.h>

#include <MqttDevice.h>

#include "apiserver.h"

#include "platform.h"
#include "utils.h"
#include "config.h"
#include "html.h"
#include "mqttview.h"

const uint WATCHDOG_TIMEOUT_S = 30;
const uint WIFI_DISCONNECT_FORCED_RESTART_S = 60;

WiFiClient net;
PubSubClient client(net);
MqttView g_mqttView(&client);

Settings g_settings;
ApiServer server(&g_settings);

#ifdef SUPPORT_RGB_LED
#include "led_rgb.h"
Led *g_led = new LedRGB(RGB_LED_PIN);
#else
#include "led_builtin.h"
Led *g_led = new LedBuiltin(LED_BUILTIN);
#endif

const char *HOMEASSISTANT_STATUS_TOPIC = "homeassistant/status";
const char *HOMEASSISTANT_STATUS_TOPIC_ALT = "ha/status";

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

bool connectToMqtt()
{
    if (client.connected())
    {
        return true;
    }

    log_info("Connecting to MQTT...");
    if (strlen(MQTT_USER) == 0)
    {
        if (!client.connect(composeClientID().c_str()))
        {
            return false;
        }
    }
    else
    {
        if (!client.connect(composeClientID().c_str(), MQTT_USER, MQTT_PASS))
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
    g_mqttView.publishConfig(g_settings);

    return true;
}

bool connectToWifi()
{
    return WiFi.status() == WL_CONNECTED;
}

void openDoor()
{
    delay(50);
    tcsWriter.write(g_settings.getCodeSettings().codeDoorOpener);
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
        g_commandToSend = g_settings.getCodeSettings().codeEntryDoorBell;
        g_shouldSend = true;
    }
    else if (strcmp(topic, g_mqttView.getApartmentBell().getCommandTopic()) == 0)
    {
        g_commandToSend = g_settings.getCodeSettings().codeApartmentDoorBell;
        g_shouldSend = true;
    }
    else if (strcmp(topic, g_mqttView.getEntryOpener().getCommandTopic()) == 0)
    {
        g_commandToSend = g_settings.getCodeSettings().codeDoorOpener;
        g_shouldSend = true;
    }

    else if (strcmp(topic, g_mqttView.getPartyMode().getCommandTopic()) == 0)
    {
        Settings::GeneralSettings generalSettings = g_settings.getGeneralSettings();
        generalSettings.partyMode = strncmp((char *)payload, g_mqttView.getPartyMode().getOnState(), length) == 0;
        g_settings.setGeneralSettings(generalSettings);
        g_settings.save();

        g_led->setBackgroundLight(generalSettings.partyMode);
        g_led->blinkAsync(); // force update of led
        g_mqttView.publishPartyMode(generalSettings.partyMode);
    }

    // Save config entries from Homeassistant
    else if (strcmp(topic, g_mqttView.getConfigCodeApartmentDoorBell().getCommandTopic()) == 0)
    {
        Settings::CodeSettings codeSettings = g_settings.getCodeSettings();
        codeSettings.codeApartmentDoorBell = parseValue((char *)payload, length);
        g_settings.setCodeSettings(codeSettings);
        g_settings.save();
        g_mqttView.publishConfigValues(g_settings);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeEntryDoorBell().getCommandTopic()) == 0)
    {
        Settings::CodeSettings codeSettings = g_settings.getCodeSettings();
        codeSettings.codeEntryDoorBell = parseValue((char *)payload, length);
        g_settings.setCodeSettings(codeSettings);
        g_settings.save();
        g_mqttView.publishConfigValues(g_settings);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeHandsetLiftup().getCommandTopic()) == 0)
    {
        Settings::CodeSettings codeSettings = g_settings.getCodeSettings();
        codeSettings.codeHandsetLiftup = parseValue((char *)payload, length);
        g_settings.setCodeSettings(codeSettings);
        g_settings.save();
        g_mqttView.publishConfigValues(g_settings);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeDoorOpener().getCommandTopic()) == 0)
    {
        Settings::CodeSettings codeSettings = g_settings.getCodeSettings();
        codeSettings.codeDoorOpener = parseValue((char *)payload, length);
        g_settings.setCodeSettings(codeSettings);
        g_settings.save();
        g_mqttView.publishConfigValues(g_settings);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeApartmentPatternDetect().getCommandTopic()) == 0)
    {
        Settings::CodeSettings codeSettings = g_settings.getCodeSettings();
        codeSettings.codeApartmentPatternDetect = parseValue((char *)payload, length);
        g_settings.setCodeSettings(codeSettings);
        g_settings.save();
        g_mqttView.publishConfigValues(g_settings);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodeEntryPatternDetect().getCommandTopic()) == 0)
    {
        Settings::CodeSettings codeSettings = g_settings.getCodeSettings();
        codeSettings.codeEntryPatternDetect = parseValue((char *)payload, length);
        g_settings.setCodeSettings(codeSettings);
        g_settings.save();
        g_mqttView.publishConfigValues(g_settings);
    }
    else if (strcmp(topic, g_mqttView.getConfigCodePartyMode().getCommandTopic()) == 0)
    {
        Settings::CodeSettings codeSettings = g_settings.getCodeSettings();
        codeSettings.codePartyMode = parseValue((char *)payload, length);
        g_settings.setCodeSettings(codeSettings);
        g_settings.save();
        g_mqttView.publishConfigValues(g_settings);
    }

    else if (strcmp(topic, g_mqttView.getDiagnosticsResetButton().getCommandTopic()) == 0)
    {
        bool pressed = strncmp((char *)payload, g_mqttView.getDiagnosticsResetButton().getPressState(), length) == 0;
        Settings::GeneralSettings generalSettings = g_settings.getGeneralSettings();
        generalSettings.restartCounter = 0;
        generalSettings.mqttDisconnectCounter = 0;
        generalSettings.wifiDisconnectCounter = 0;
        g_settings.setGeneralSettings(generalSettings);
        g_settings.save();
        g_mqttView.publishDiagnostics(g_settings, g_bssid.c_str());
    }

    // publish config when homeassistant comes online and needs the configuration again
    else if (strcmp(topic, HOMEASSISTANT_STATUS_TOPIC) == 0 ||
             strcmp(topic, HOMEASSISTANT_STATUS_TOPIC_ALT) == 0)
    {
        if (strncmp((char *)payload, "online", length) == 0)
        {
            g_mqttView.publishConfig(g_settings);
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
    Serial.begin(115200);

    g_led->begin();
    // turn on led until boot sequence finished
    g_led->blinkAsync();

    delay(400);
    log_info("Starting to mount LittleFS");
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
    log_info("Finished Mounting");

    g_settings.begin();
    Settings::GeneralSettings generalSettings = g_settings.getGeneralSettings();
    generalSettings.restartCounter++;
    g_settings.setGeneralSettings(generalSettings);
    g_settings.save();

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

    //WiFi.begin(g_settings.getWiFiSettings().staSsid, g_settings.getWiFiSettings().staPassword);
    WiFi.begin(DEFAULT_STA_WIFI_SSID, DEFAULT_STA_WIFI_PASS);

    log_info("Connecting to wifi...");
    // TODO: really forever? What if we want to go back to autoconnect?
    long startTime = millis();
    while (!connectToWifi())
    {
        if (millis() - startTime > WIFI_CONNECTION_FAIL_TIMEOUT_S*1000)
        {
            // we failed to connect to the wifi, force reboot in AP settings
            Settings::GeneralSettings settings = g_settings.getGeneralSettings();
            settings.forceAPnextBoot = true;
            g_settings.setGeneralSettings(settings);
            g_settings.save();
            ESP.restart();
        }
        log_debug(".");
        delay(500);
    }
    g_wifiConnected = true;
    g_lastWifiConnect = millis();

    log_info("Wifi connected!");

    log_info("Connected to SSID: %s", g_settings.getWiFiSettings().staSsid);
    log_info("IP address: %s", WiFi.localIP().toString().c_str());
    g_bssid = WiFi.BSSIDstr();

    char configUrl[256];
    snprintf(configUrl, sizeof(configUrl), "http://%s/", WiFi.localIP().toString().c_str());
    g_mqttView.getDevice().setConfigurationUrl(configUrl);

    client.setBufferSize(1024);
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
    server.begin(); // Start the server
    log_info("Server listening");

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
            Settings::GeneralSettings generalSettings = g_settings.getGeneralSettings();
            generalSettings.wifiDisconnectCounter++;
            g_settings.setGeneralSettings(generalSettings);
            g_settings.save();
        }
        if (millis() - g_lastWifiConnect > WIFI_DISCONNECT_FORCED_RESTART_S * 1000)
        {
            log_warn("Wifi could not connect in time, will force a restart");
            ESP.restart();
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
            Settings::GeneralSettings generalSettings = g_settings.getGeneralSettings();
            generalSettings.mqttDisconnectCounter++;
            g_settings.setGeneralSettings(generalSettings);
            g_settings.save();
        }
        g_mqttConnected = false;
        delay(1000);
        return;
    }
    if (!g_mqttConnected)
    {
        // now we are successfully reconnected and publish our counters
        g_bssid = WiFi.BSSIDstr();
        g_mqttView.publishDiagnostics(g_settings, g_bssid.c_str());
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

        if (cmd == g_settings.getCodeSettings().codeApartmentDoorBell)
        {
            g_mqttView.publishApartmentBellTrigger();
        }

        if (cmd == g_settings.getCodeSettings().codeEntryDoorBell)
        {
            g_mqttView.publishEntryBellTrigger();
        }

        if (cmd == g_settings.getCodeSettings().codeDoorOpener)
        {
            g_mqttView.publishEntryOpenerTrigger();
        }

        if (g_settings.getGeneralSettings().partyMode && cmd == g_settings.getCodeSettings().codePartyMode)
        {
            // we have a party, let everybody in
            openDoor();
        }

        if (cmd == g_settings.getCodeSettings().codeHandsetLiftup)
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
                Settings::GeneralSettings generalSettings = g_settings.getGeneralSettings();
                generalSettings.partyMode = !generalSettings.partyMode;
                g_settings.setGeneralSettings(generalSettings);
                g_settings.save();
                g_handsetLiftup = 0;
                g_led->setBackgroundLight(generalSettings.partyMode);
                g_led->blinkAsync();
                g_mqttView.publishPartyMode(generalSettings.partyMode);
            }
        }

        if (cmd == g_settings.getCodeSettings().codeEntryPatternDetect)
        {
            if (patternRecognitionEntry.trigger())
            {
                // TODO: make pattern configurable and enableable
                openDoor();
                g_mqttView.publishEntryBellPatternTrigger();
            }
        }

        if (cmd == g_settings.getCodeSettings().codeApartmentPatternDetect)
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
