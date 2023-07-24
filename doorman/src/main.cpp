#include <Arduino.h>

// needed for library
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

#include "utils.h"
#include "config.h"
#include "html.h"

#ifdef ESP8266
#define PIN_BUS_READ D5
#define PIN_BUS_WRITE D6
#define SYSTEM_NAME "ESP8266 Doorman"
#endif

#ifdef ESP32
#define PIN_BUS_READ 12
#define PIN_BUS_WRITE 13
#define SYSTEM_NAME "ESP32 Doorman"
#endif

#define CONFIG_FILENAME "/config.txt"

WiFiClient net;
PubSubClient client(net);

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
};

Config g_config = {0, 0, 0, 0, 0, 0, 0, 0};

// apartement door:
//   doorman-[name]/apartment/bell/state -> on/off
//   doorman-[name]/apartment/bell/pattern/state -> on/off
// entry door:
//   doorman-[name]/entry/bell/state -> on/off
//   doorman-[name]/entry/bell/pattern/state -> on/off
//   doorman-[name]/entry/opener/cmd
//   doorman-[name]/partymode/state -> on/off

// commands
// 0x1100 door opener if the handset is not lifted up
// 0x1180 door opener if the handset is lifted up

// 0x1B8F9A41 own door bell at the flat door	feilipu/FreeRTOS@^10.4.6-1
// 0x0B8F9A80 own door bell at the main door
MqttDevice mqttDevice(composeClientID().c_str(), "Doorman", SYSTEM_NAME, "maker_pt");
MqttSwitch mqttApartmentBell(&mqttDevice, "apartmentbell", "Apartment Bell");
MqttBinarySensor mqttApartmentBellPattern(&mqttDevice, "apartmentbellpattern", "Apartment Bell Pattern");

MqttSwitch mqttEntryBell(&mqttDevice, "entrybell", "Entry Bell");
MqttBinarySensor mqttEntryBellPattern(&mqttDevice, "entrybellpattern", "Entry Bell Pattern");
MqttSwitch mqttEntryOpener(&mqttDevice, "entryopener", "Door Opener");

MqttSwitch mqttPartyMode(&mqttDevice, "partymode", "Door Opener Party Mode");

MqttText mqttBus(&mqttDevice, "bus", "TCS Bus");

// Configuration Settings
MqttText mqttConfigCodeApartmentDoorBell(&mqttDevice, "config_code_apartment_door_bell", "Apartment Door Bell Code");
MqttText mqttConfigCodeEntryDoorBell(&mqttDevice, "config_code_entry_door_bell", "Entry Door Bell Code");
MqttText mqttConfigCodeHandsetLiftup(&mqttDevice, "config_code_handset_liftup", "Handset Liftup Code");
MqttText mqttConfigCodeDoorOpener(&mqttDevice, "config_code_door_opener", "Entry Door Opener Code");
MqttText mqttConfigCodeApartmentPatternDetect(&mqttDevice, "config_code_apartment_pattern_detected", "Apartment Pattern Detected Code");
MqttText mqttConfigCodeEntryPatternDetect(&mqttDevice, "config_code_entry_pattern_detected", "Entry Pattern Detected Code");
MqttText mqttConfigCodePartyMode(&mqttDevice, "config_code_party_mode", "Party Mode Code");

MqttSensor mqttDiagnosticsRestartCounter(&mqttDevice, "diagnostics_restart_counter", "Doorman Restart Counter");

bool g_partyMode = false;

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

// TODO: wifi auto config
// TODO: publish persistant

void publishMqttState(MqttEntity *entity, const char *state)
{
    if (!client.publish(entity->getStateTopic(), state))
    {
        log_error("Failed to publish state to %s", entity->getStateTopic());
    }
}

void publishMqttConfigState(MqttEntity *entity, const uint32_t value)
{
    char state[9];
    snprintf(state, sizeof(state), "%08x", value);
    if (!client.publish(entity->getStateTopic(), state))
    {
        log_error("Failed to publish state to %s", entity->getStateTopic());
    }
}

void publishMqttRestartCounterState(MqttEntity *entity, const uint32_t value)
{
    char state[9];
    snprintf(state, sizeof(state), "%u", value);
    client.publish(entity->getStateTopic(), state);
}

void publishOnOffEdgeSwitch(MqttSwitch *entity)
{
    publishMqttState(entity, entity->getOnState());
    delay(1000);
    publishMqttState(entity, entity->getOffState());
}

void publishOnOffEdgeBinary(MqttBinarySensor *entity)
{
    publishMqttState(entity, entity->getOnState());
    delay(1000);
    publishMqttState(entity, entity->getOffState());
}

void publishOnOffEdgeLock(MqttLock *entity)
{
    publishMqttState(entity, entity->getUnlockedState());
    delay(1000);
    publishMqttState(entity, entity->getLockedState());
}

void publishPartyMode()
{
    publishMqttState(&mqttPartyMode, g_partyMode ? mqttPartyMode.getOnState() : mqttPartyMode.getOffState());
}

void publishConfigValues()
{
    publishMqttConfigState(&mqttConfigCodeApartmentDoorBell, g_config.codeApartmentDoorBell);
    publishMqttConfigState(&mqttConfigCodeEntryDoorBell, g_config.codeEntryDoorBell);
    publishMqttConfigState(&mqttConfigCodeHandsetLiftup, g_config.codeHandsetLiftup);
    publishMqttConfigState(&mqttConfigCodeDoorOpener, g_config.codeDoorOpener);
    publishMqttConfigState(&mqttConfigCodeApartmentPatternDetect, g_config.codeApartmentPatternDetect);
    publishMqttConfigState(&mqttConfigCodeEntryPatternDetect, g_config.codeEntryPatternDetect);
    publishMqttConfigState(&mqttConfigCodePartyMode, g_config.codePartyMode);
    publishMqttRestartCounterState(&mqttDiagnosticsRestartCounter, g_config.restartCounter);
}

void publishConfig(MqttEntity *entity)
{
    String payload = entity->getHomeAssistantConfigPayload();
    char topic[255];
    entity->getHomeAssistantConfigTopic(topic, sizeof(topic));
    if (!client.publish(topic, payload.c_str()))
    {
        log_error("Failed to publish config to %s", entity->getStateTopic());
    }
    entity->getHomeAssistantConfigTopicAlt(topic, sizeof(topic));
    if (!client.publish(topic, payload.c_str()))
    {
        log_error("Failed to publish config to %s", entity->getStateTopic());
    }
}

void publishConfig()
{
    publishConfig(&mqttApartmentBell);
    publishConfig(&mqttApartmentBellPattern);

    publishConfig(&mqttEntryBell);
    publishConfig(&mqttEntryBellPattern);
    publishConfig(&mqttEntryOpener);

    publishConfig(&mqttPartyMode);

    publishConfig(&mqttBus);

    publishConfig(&mqttConfigCodeApartmentDoorBell);
    publishConfig(&mqttConfigCodeEntryDoorBell);
    publishConfig(&mqttConfigCodeHandsetLiftup);
    publishConfig(&mqttConfigCodeDoorOpener);
    publishConfig(&mqttConfigCodeApartmentPatternDetect);
    publishConfig(&mqttConfigCodeEntryPatternDetect);
    publishConfig(&mqttConfigCodePartyMode);
    publishConfig(&mqttDiagnosticsRestartCounter);

    delay(1000);
    // publish all initial states
    publishMqttState(&mqttApartmentBell, mqttApartmentBell.getOffState());
    publishMqttState(&mqttApartmentBellPattern, mqttApartmentBellPattern.getOffState());

    publishMqttState(&mqttEntryBell, mqttEntryBell.getOffState());
    publishMqttState(&mqttEntryBellPattern, mqttEntryBellPattern.getOffState());
    publishMqttState(&mqttEntryOpener, mqttEntryOpener.getOffState());
    publishMqttState(&mqttBus, "");
    publishPartyMode();

    publishConfigValues();
}

void connectToMqtt()
{
    log_info("Connecting to MQTT...");
    if (strlen(mqtt_user) == 0)
    {
        while (!client.connect(composeClientID().c_str()))
        {
            log_debug(".");
            delay(4000);
        }
    }
    else
    {
        while (!client.connect(composeClientID().c_str(), mqtt_user, mqtt_pass))
        {
            log_debug(".");
            delay(4000);
        }
    }

    client.subscribe(mqttApartmentBell.getCommandTopic(), 1);

    client.subscribe(mqttEntryBell.getCommandTopic(), 1);
    client.subscribe(mqttEntryOpener.getCommandTopic(), 1);

    client.subscribe(mqttPartyMode.getCommandTopic(), 1);

    client.subscribe(mqttBus.getCommandTopic(), 1);

    client.subscribe(mqttConfigCodeApartmentDoorBell.getCommandTopic(), 1);
    client.subscribe(mqttConfigCodeEntryDoorBell.getCommandTopic(), 1);
    client.subscribe(mqttConfigCodeHandsetLiftup.getCommandTopic(), 1);
    client.subscribe(mqttConfigCodeDoorOpener.getCommandTopic(), 1);
    client.subscribe(mqttConfigCodeApartmentPatternDetect.getCommandTopic(), 1);
    client.subscribe(mqttConfigCodeEntryPatternDetect.getCommandTopic(), 1);
    client.subscribe(mqttConfigCodePartyMode.getCommandTopic(), 1);

    client.subscribe(HOMEASSISTANT_STATUS_TOPIC);
    client.subscribe(HOMEASSISTANT_STATUS_TOPIC_ALT);

    // TODO: solve this somehow with auto discovery lib
    // client.publish(mqttTopic(MQTT_TOPIC_NONE, MQTT_ACTION_NONE).c_str(), "online");
    publishConfig();
}

void connectToWifi()
{
    log_info("Connecting to wifi...");
    // TODO: really forever? What if we want to go back to autoconnect?
    while (WiFi.status() != WL_CONNECTED)
    {
        log_debug(".");
        delay(1000);
    }
    log_info("Wifi connected!");
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
}

void loadSettings()
{
    // Open file for reading
    // TODO: check if file exists
    File file = LittleFS.open(CONFIG_FILENAME, "r");

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<1024> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        log_error(F("Failed to read file, using default configuration"));
    }

    // Copy values from the JsonDocument to the Config
    g_config.codeApartmentDoorBell = doc["codeApartmentDoorBell"] | g_config.codeApartmentDoorBell;
    g_config.codeEntryDoorBell = doc["codeEntryDoorBell"] | g_config.codeEntryDoorBell;
    g_config.codeHandsetLiftup = doc["codeHandsetLiftup"] | g_config.codeHandsetLiftup;
    g_config.codeDoorOpener = doc["codeDoorOpener"] | g_config.codeDoorOpener;
    g_config.codeApartmentPatternDetect = doc["codeApartmentPatternDetect"] | g_config.codeApartmentPatternDetect;
    g_config.codeEntryPatternDetect = doc["codeEntryPatternDetect"] | g_config.codeEntryPatternDetect;
    g_config.codePartyMode = doc["codePartyMode"] | g_config.codePartyMode;
    g_config.restartCounter = doc["restartCounter"] | g_config.restartCounter;

    // Close the file (Curiously, File's destructor doesn't close the file)
    file.close();
}

void saveSettings()
{
    // Open file for writing
    File file = LittleFS.open(CONFIG_FILENAME, "w");
    if (!file)
    {
        log_error("Failed to create config file");
        return;
    }
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
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

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0)
    {
        log_error("Failed to write config to file");
    }

    // Close the file
    file.close();
}

void handleGetConfig()
{
    // Respond with the current configuration in JSON format
    String configJson;
    StaticJsonDocument<1024> doc;

    // Set the values in the document
    doc["CodeApartmentDoorBell"] = g_config.codeApartmentDoorBell;
    doc["CodeEntryDoorBell"] = g_config.codeEntryDoorBell;
    doc["CodeHandsetLiftup"] = g_config.codeHandsetLiftup;
    doc["CodeDoorOpener"] = g_config.codeDoorOpener;
    doc["CodeApartmentPatternDetect"] = g_config.codeApartmentPatternDetect;
    doc["CodeEntryPatternDetect"] = g_config.codeEntryPatternDetect;
    doc["CodePartyMode"] = g_config.codePartyMode;
    doc["RestartCounter"] = g_config.restartCounter;
    doc["Version"] = SYSTEM_NAME " (" __DATE__ ")";

    serializeJson(doc, configJson);
    server.send(200, "application/json", configJson);
}

void handleSaveConfig()
{
    // Handle form submission and update configuration data
    if (server.method() == HTTP_POST)
    {
        StaticJsonDocument<1024> doc;
        String jsonString = server.arg("plain");
        deserializeJson(doc, jsonString);

        // Update the currentConfig struct with the new values from the form submission
        // Copy values from the JsonDocument to the Config
        g_config.codeApartmentDoorBell = doc["CodeApartmentDoorBell"] | g_config.codeApartmentDoorBell;
        g_config.codeEntryDoorBell = doc["CodeEntryDoorBell"] | g_config.codeEntryDoorBell;
        g_config.codeHandsetLiftup = doc["CodeHandsetLiftup"] | g_config.codeHandsetLiftup;
        g_config.codeDoorOpener = doc["CodeDoorOpener"] | g_config.codeDoorOpener;
        g_config.codeApartmentPatternDetect = doc["CodeApartmentPatternDetect"] | g_config.codeApartmentPatternDetect;
        g_config.codeEntryPatternDetect = doc["CodeEntryPatternDetect"] | g_config.codeEntryPatternDetect;
        g_config.codePartyMode = doc["CodePartyMode"] | g_config.codePartyMode;
        saveSettings();
        publishConfigValues();

        // Send a response to the client
        String responseMessage = "Configuration updated successfully!";
        server.send(200, "application/json", "{\"message\":\"" + responseMessage + "\"}");
    }
}

void handleSettingsPage()
{
    server.send_P(200, "text/html", PAGE_SETTINGS); //, sizeof(PAGE_SETTINGS));
}

bool formatLittleFS()
{
    log_warn("need to format LittleFS: ");
    LittleFS.end();
    LittleFS.begin();
    log_info("Success: %d", LittleFS.format());
    return LittleFS.begin();
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

    if (strcmp(topic, mqttBus.getCommandTopic()) == 0)
    {
        uint32_t data = parseValue((char *)payload, length);
        g_commandToSend = data;
        g_shouldSend = true;
    }
    else if (strcmp(topic, mqttEntryBell.getCommandTopic()) == 0)
    {
        g_commandToSend = g_config.codeEntryDoorBell;
        g_shouldSend = true;
    }
    else if (strcmp(topic, mqttApartmentBell.getCommandTopic()) == 0)
    {
        g_commandToSend = g_config.codeApartmentDoorBell;
        g_shouldSend = true;
    }
    else if (strcmp(topic, mqttEntryOpener.getCommandTopic()) == 0)
    {
        g_commandToSend = g_config.codeDoorOpener;
        g_shouldSend = true;
    }

    else if (strcmp(topic, mqttPartyMode.getCommandTopic()) == 0)
    {
        g_partyMode = strncmp((char *)payload, mqttPartyMode.getOnState(), length) == 0;
        g_led->setBackgroundLight(g_partyMode);
        g_led->blinkAsync(); // force update of led
        publishPartyMode();
    }

    // Save config entries from Homeassistant
    else if (strcmp(topic, mqttConfigCodeApartmentDoorBell.getCommandTopic()) == 0)
    {
        g_config.codeApartmentDoorBell = parseValue((char *)payload, length);
        publishMqttConfigState(&mqttConfigCodeApartmentDoorBell, g_config.codeApartmentDoorBell);
        saveSettings();
    }
    else if (strcmp(topic, mqttConfigCodeEntryDoorBell.getCommandTopic()) == 0)
    {
        g_config.codeEntryDoorBell = parseValue((char *)payload, length);
        saveSettings();
        publishMqttConfigState(&mqttConfigCodeEntryDoorBell, g_config.codeEntryDoorBell);
    }
    else if (strcmp(topic, mqttConfigCodeHandsetLiftup.getCommandTopic()) == 0)
    {
        g_config.codeHandsetLiftup = parseValue((char *)payload, length);
        saveSettings();
        publishMqttConfigState(&mqttConfigCodeHandsetLiftup, g_config.codeHandsetLiftup);
    }
    else if (strcmp(topic, mqttConfigCodeDoorOpener.getCommandTopic()) == 0)
    {
        g_config.codeDoorOpener = parseValue((char *)payload, length);
        saveSettings();
        publishMqttConfigState(&mqttConfigCodeDoorOpener, g_config.codeDoorOpener);
    }
    else if (strcmp(topic, mqttConfigCodeApartmentPatternDetect.getCommandTopic()) == 0)
    {
        g_config.codeApartmentPatternDetect = parseValue((char *)payload, length);
        saveSettings();
        publishMqttConfigState(&mqttConfigCodeApartmentPatternDetect, g_config.codeApartmentPatternDetect);
    }
    else if (strcmp(topic, mqttConfigCodeEntryPatternDetect.getCommandTopic()) == 0)
    {
        g_config.codeEntryPatternDetect = parseValue((char *)payload, length);
        saveSettings();
        publishMqttConfigState(&mqttConfigCodeEntryPatternDetect, g_config.codeEntryPatternDetect);
    }
    else if (strcmp(topic, mqttConfigCodePartyMode.getCommandTopic()) == 0)
    {
        g_config.codePartyMode = parseValue((char *)payload, length);
        saveSettings();
        publishMqttConfigState(&mqttConfigCodePartyMode, g_config.codePartyMode);
    }

    // publish config when homeassistant comes online and needs the configuration again
    else if (strcmp(topic, HOMEASSISTANT_STATUS_TOPIC) == 0 ||
             strcmp(topic, HOMEASSISTANT_STATUS_TOPIC_ALT) == 0)
    {
        if (strncmp((char *)payload, "online", length) == 0)
        {
            publishConfig();
        }
    }
}

void setup()
{
    // further mqtt device config
    mqttBus.setPattern("[a-fA-F0-9]*");
    mqttBus.setMaxLetters(8);
    mqttBus.setIcon("mdi:console-network");

    mqttApartmentBell.setIcon("mdi:bell");
    mqttEntryBell.setIcon("mdi:bell");

    mqttPartyMode.setIcon("mdi:door-closed-lock");
    mqttEntryOpener.setIcon("mdi:door-open");

    mqttConfigCodeApartmentDoorBell.setPattern("[a-fA-F0-9]*");
    mqttConfigCodeApartmentDoorBell.setMaxLetters(8);
    mqttConfigCodeApartmentDoorBell.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeEntryDoorBell.setPattern("[a-fA-F0-9]*");
    mqttConfigCodeEntryDoorBell.setMaxLetters(8);
    mqttConfigCodeEntryDoorBell.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeHandsetLiftup.setPattern("[a-fA-F0-9]*");
    mqttConfigCodeHandsetLiftup.setMaxLetters(8);
    mqttConfigCodeHandsetLiftup.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeDoorOpener.setPattern("[a-fA-F0-9]*");
    mqttConfigCodeDoorOpener.setMaxLetters(8);
    mqttConfigCodeDoorOpener.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeApartmentPatternDetect.setPattern("[a-fA-F0-9]*");
    mqttConfigCodeApartmentPatternDetect.setMaxLetters(8);
    mqttConfigCodeApartmentPatternDetect.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeEntryPatternDetect.setPattern("[a-fA-F0-9]*");
    mqttConfigCodeEntryPatternDetect.setMaxLetters(8);
    mqttConfigCodeEntryPatternDetect.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodePartyMode.setPattern("[a-fA-F0-9]*");
    mqttConfigCodePartyMode.setMaxLetters(8);
    mqttConfigCodePartyMode.setEntityType(EntityCategory::CONFIG);

    mqttDiagnosticsRestartCounter.setEntityType(EntityCategory::DIAGNOSTIC);
    mqttDiagnosticsRestartCounter.setStateClass(MqttSensor::StateClass::TOTAL_INCREASING);

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
    loadSettings();
    g_config.restartCounter++;
    saveSettings();

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
    WiFi.begin(wifi_ssid, wifi_pass);

    connectToWifi();

    log_info("Connected to SSID: %s", wifi_ssid);
    log_info("IP address: %s", WiFi.localIP().toString().c_str());
    char configUrl[256];
    snprintf(configUrl, sizeof(configUrl), "http://%s/", WiFi.localIP().toString().c_str());
    mqttDevice.setConfigurationUrl(configUrl);

    client.setBufferSize(1024);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    server.on("/", handleSettingsPage); // Associate the handler function to the path
    server.on("/setConfig", handleSaveConfig);
    server.on("/getConfig", handleGetConfig);
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
                          { log_info("Progress: %u%%\r", (progress / (total / 100))); });
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

    connectToWifi();
}

void loop()
{
    g_led->update();
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWifi();
    }
    if (!client.connected())
    {
        connectToMqtt();
    }
    client.loop();
    server.handleClient(); // Handling of incoming web requests
    ArduinoOTA.handle();
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
            publishOnOffEdgeSwitch(&mqttApartmentBell);
        }

        if (cmd == g_config.codeEntryDoorBell)
        {
            publishOnOffEdgeSwitch(&mqttEntryBell);
        }

        if (cmd == g_config.codeDoorOpener)
        {
            publishOnOffEdgeSwitch(&mqttEntryOpener);
        }

        if (g_partyMode && cmd == g_config.codePartyMode)
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
                g_partyMode = !g_partyMode;
                g_handsetLiftup = 0;
                g_led->setBackgroundLight(g_partyMode);
                g_led->blinkAsync();
                publishPartyMode();
            }
        }

        if (cmd == g_config.codeEntryPatternDetect)
        {
            if (patternRecognitionEntry.trigger())
            {
                openDoor();
                publishOnOffEdgeBinary(&mqttEntryBellPattern);
            }
        }

        if (cmd == g_config.codeApartmentPatternDetect)
        {
            if (patternRecognitionApartment.trigger())
            {
                publishOnOffEdgeBinary(&mqttApartmentBellPattern);
            }
        }

        log_info("TCS Bus: %08x", cmd);

        char byte_cmd[9];
        sprintf(byte_cmd, "%08x", cmd);
        if (!client.publish(mqttBus.getStateTopic(), byte_cmd))
        {
            log_error("Failed to publish tcs data to %s", mqttBus.getStateTopic());
        }
    }
}
