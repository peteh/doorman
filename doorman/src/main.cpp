#include <Arduino.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino

// needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include <TCSBus.h>
#include <TriggerPatternRecognition.h>

#include <MqttDevice.h>

#include "utils.h"
#include "config.h"
#include "html.h"

#define PIN_BUS_READ D5
#define PIN_BUS_WRITE D6
#define CONFIG_FILENAME "/config.txt"

WiFiClient net;
PubSubClient client(net);
ESP8266WebServer server(80);

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
};

Config g_config = {0, 0, 0, 0, 0, 0, 0};

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
MqttDevice mqttDevice(composeClientID().c_str(), "Doorman", "ESP8622 Doorman", "maker_pt");
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
// TODO: publish mqtt bus as text entity in HA

void blinkLedAsync()
{
    digitalWrite(LED_BUILTIN, LOW ^ g_partyMode);
    g_tsLastLedStateOn = millis();
    g_ledState = true;
}

void publishMqttState(MqttEntity *device, const char *state)
{
    client.publish(device->getStateTopic(), state);
}

void publishMqttConfigState(MqttEntity *device, const uint32_t value)
{
    char state[9];
    snprintf(state, sizeof(state), "%08x", value);
    client.publish(device->getStateTopic(), state);
}

void publishOnOffEdgeSwitch(MqttSwitch *device)
{
    publishMqttState(device, device->getOnState());
    delay(1000);
    publishMqttState(device, device->getOffState());
}

void publishOnOffEdgeBinary(MqttBinarySensor *device)
{
    publishMqttState(device, device->getOnState());
    delay(1000);
    publishMqttState(device, device->getOffState());
}

void publishOnOffEdgeLock(MqttLock *device)
{
    publishMqttState(device, device->getUnlockedState());
    delay(1000);
    publishMqttState(device, device->getLockedState());
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
}

void publishConfig(MqttEntity *device)
{
    String payload = device->getHomeAssistantConfigPayload();
    char topic[255];
    device->getHomeAssistantConfigTopic(topic, sizeof(topic));
    client.publish(topic, payload.c_str());

    device->getHomeAssistantConfigTopicAlt(topic, sizeof(topic));
    client.publish(topic,
                   payload.c_str());
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
    Serial.print("\nconnecting to MQTT...");
    // TODO: add security settings back to mqtt
    // while (!client.connect(mqtt_client, mqtt_user, mqtt_pass))
    while (!client.connect(composeClientID().c_str()))
    {
        Serial.print(".");
        delay(4000);
    }

    client.subscribe(mqttApartmentBell.getCommandTopic(), 1);

    client.subscribe(mqttEntryBell.getCommandTopic(), 1);
    client.subscribe(mqttEntryOpener.getCommandTopic(), 1);

    client.subscribe(mqttPartyMode.getCommandTopic(), 1);

    client.subscribe(mqttBus.getCommandTopic(), 1);

    client.subscribe(HOMEASSISTANT_STATUS_TOPIC);
    client.subscribe(HOMEASSISTANT_STATUS_TOPIC_ALT);

    // TODO: solve this somehow with auto discovery lib
    // client.publish(mqttTopic(MQTT_TOPIC_NONE, MQTT_ACTION_NONE).c_str(), "online");
    publishConfig();
}

void connectToWifi()
{
    Serial.print("Connecting to wifi...");
    // TODO: really forever? What if we want to go back to autoconnect?
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\n Wifi connected!");
}

void printSettings()
{
    Serial.printf("%s: %08x\n", "Code Apartment Door Bell", g_config.codeApartmentDoorBell);
    Serial.printf("%s: %08x\n", "Code Entry Door Bell", g_config.codeEntryDoorBell);
    Serial.printf("%s: %08x\n", "Code Handset Liftup", g_config.codeHandsetLiftup);
    Serial.printf("%s: %08x\n", "Code Door Opener", g_config.codeDoorOpener);
    Serial.printf("%s: %08x\n", "Code Apartment Door Pattern Detection", g_config.codeApartmentPatternDetect);
    Serial.printf("%s: %08x\n", "Code Entry Door Pattern Detection", g_config.codeEntryPatternDetect);
    Serial.printf("%s: %08x\n", "Code Party Mode", g_config.codePartyMode);
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
        Serial.println(F("Failed to read file, using default configuration"));
    }

    // Copy values from the JsonDocument to the Config
    g_config.codeApartmentDoorBell = doc["codeApartmentDoorBell"] | g_config.codeApartmentDoorBell;
    g_config.codeEntryDoorBell = doc["codeEntryDoorBell"] | g_config.codeEntryDoorBell;
    g_config.codeHandsetLiftup = doc["codeHandsetLiftup"] | g_config.codeHandsetLiftup;
    g_config.codeDoorOpener = doc["codeDoorOpener"] | g_config.codeDoorOpener;
    g_config.codeApartmentPatternDetect = doc["codeApartmentPatternDetect"] | g_config.codeApartmentPatternDetect;
    g_config.codeEntryPatternDetect = doc["codeEntryPatternDetect"] | g_config.codeEntryPatternDetect;
    g_config.codePartyMode = doc["codePartyMode"] | g_config.codePartyMode;

    // Close the file (Curiously, File's destructor doesn't close the file)
    file.close();
}

void saveSettings()
{
    // Open file for writing
    File file = LittleFS.open(CONFIG_FILENAME, "w");
    if (!file)
    {
        Serial.println("Failed to create file");
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

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0)
    {
        Serial.println("Failed to write to file");
    }

    // Close the file
    file.close();
}

void handleSettingsPage()
{
    if (server.hasArg("save"))
    { // Check if body received
        g_config.codeApartmentDoorBell = (int)strtol(server.arg("CodeApartmentDoorBell").c_str(), 0, 16);
        g_config.codeEntryDoorBell = (int)strtol(server.arg("CodeEntryDoorBell").c_str(), 0, 16);
        g_config.codeHandsetLiftup = (int)strtol(server.arg("CodeHandsetLiftup").c_str(), 0, 16);
        g_config.codeDoorOpener = (int)strtol(server.arg("CodeDoorOpener").c_str(), 0, 16);
        g_config.codeApartmentPatternDetect = (int)strtol(server.arg("CodeApartmentPatternDetect").c_str(), 0, 16);
        g_config.codeEntryPatternDetect = (int)strtol(server.arg("CodeEntryPatternDetect").c_str(), 0, 16);
        g_config.codePartyMode = (int)strtol(server.arg("CodePartyMode").c_str(), 0, 16);
        printSettings();
        saveSettings();
    }
    char buffer[2000];
    snprintf(buffer, sizeof(buffer), PAGE_SETTINGS,
             g_config.codeApartmentDoorBell,
             g_config.codeEntryDoorBell,
             g_config.codeHandsetLiftup,
             g_config.codeDoorOpener,
             g_config.codeApartmentPatternDetect,
             g_config.codeEntryPatternDetect,
             g_config.codePartyMode);

    String message(buffer);
    server.send(200, "text/html", message);
}

bool formatLittleFS()
{
    Serial.println("need to format LittleFS: ");
    LittleFS.end();
    LittleFS.begin();
    Serial.println(LittleFS.format());
    return LittleFS.begin();
}

void openDoor()
{
    delay(50);
    tcsWriter.write(g_config.codeDoorOpener);
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    // TODO length check

    if (strcmp(topic, mqttBus.getCommandTopic()) == 0)
    {
        char temp[32];
        strncpy(temp, (char *)payload, length);
        uint32_t data = (uint32_t)strtoul(temp, NULL, 16);
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
        blinkLedAsync(); // force update of led
        publishPartyMode();
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

    mqttConfigCodeApartmentDoorBell.setMaxLetters(8);
    mqttConfigCodeApartmentDoorBell.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeEntryDoorBell.setMaxLetters(8);
    mqttConfigCodeEntryDoorBell.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeHandsetLiftup.setMaxLetters(8);
    mqttConfigCodeHandsetLiftup.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeDoorOpener.setMaxLetters(8);
    mqttConfigCodeDoorOpener.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeApartmentPatternDetect.setMaxLetters(8);
    mqttConfigCodeApartmentPatternDetect.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodeEntryPatternDetect.setMaxLetters(8);
    mqttConfigCodeEntryPatternDetect.setEntityType(EntityCategory::CONFIG);

    mqttConfigCodePartyMode.setMaxLetters(8);
    mqttConfigCodePartyMode.setEntityType(EntityCategory::CONFIG);

    pinMode(LED_BUILTIN, OUTPUT);
    // turn on led until boot sequence finished
    blinkLedAsync();

    Serial.begin(115200);

    if (!LittleFS.begin())
    {
        Serial.println("Failed to mount file system");
        delay(5000);
        if (!formatLittleFS())
        {
            Serial.println("Failed to format file system - hardware issues!");
            for (;;)
            {
                delay(100);
            }
        }
    }
    loadSettings();

    tcsWriter.begin();
    tcsReader.begin();

    // configure pattern detection
    patternRecognitionEntry.addStep(1000);
    patternRecognitionEntry.addStep(1000);

    // configure pattern detection
    patternRecognitionApartment.addStep(1000);
    patternRecognitionApartment.addStep(1000);

    WiFi.mode(WIFI_STA);
    WiFi.hostname(composeClientID().c_str());
    WiFi.begin(wifi_ssid, wifi_pass);

    connectToWifi();

    Serial.println();
    Serial.print("Connected to SSID: ");
    Serial.println(wifi_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    char configUrl[256];
    snprintf(configUrl, sizeof(configUrl), "http://%s/", WiFi.localIP().toString().c_str());
    mqttDevice.setConfigurationUrl(configUrl);

    client.setBufferSize(512);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    server.on("/", handleSettingsPage); // Associate the handler function to the path
    server.begin();                     // Start the server
    Serial.println("Server listening");

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
    Serial.println("Start updating " + type); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    } });
    ArduinoOTA.begin();

    connectToWifi();
}

void loop()
{
    if (g_ledState)
    {
        if (millis() - g_tsLastLedStateOn > 50)
        {
            g_ledState = false;
            // we invert with party mode to make the led constantly light if it's enabled
            digitalWrite(LED_BUILTIN, HIGH ^ g_partyMode);
        }
    }
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
        Serial.printf("Sending: %08x", cmd);
        Serial.println();
        tcsReader.disable();
        tcsWriter.write(cmd);
        tcsReader.enable();
        // dirty hack to also publish commands we have written
        tcsReader.inject(cmd);
        blinkLedAsync();
    }
    if (tcsReader.hasCommand())
    {
        blinkLedAsync();
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
                blinkLedAsync();
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

        Serial.print("TCS Bus: 0x");
        printHEX(cmd);
        Serial.println();

        char byte_cmd[9];
        sprintf(byte_cmd, "%08x", cmd);
        client.publish(mqttBus.getStateTopic(), byte_cmd);
    }
}
