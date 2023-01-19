#include <Arduino.h>
#include <FS.h> //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino

// needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>
#include <TCSBus.h>
#include <TriggerPatternRecognition.h>

#include <MqttDevice.h>

#include "utils.h"
#include "config.h"

#define PIN_BUS_READ D5
#define PIN_BUS_WRITE D6

WiFiClient net;
PubSubClient client(net);

const char *HOMEASSISTANT_STATUS_TOPIC = "homeassistant/status";
const char *HOMEASSISTANT_STATUS_TOPIC_ALT = "ha/status";

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

void publishMqttState(MqttEntity* device, const char *state)
{
    client.publish(device->getStateTopic(), state);
}


void publishOnOffEdgeSwitch(MqttSwitch* device)
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

    delay(1000);
    // publish all initial states
    publishMqttState(&mqttApartmentBell, mqttApartmentBell.getOffState());
    publishMqttState(&mqttApartmentBellPattern, mqttApartmentBellPattern.getOffState());

    publishMqttState(&mqttEntryBell, mqttEntryBell.getOffState());
    publishMqttState(&mqttEntryBellPattern, mqttEntryBellPattern.getOffState());
    publishMqttState(&mqttEntryOpener, mqttEntryOpener.getOffState());
    publishMqttState(&mqttBus, "");
    publishPartyMode();
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


void openDoor()
{
    delay(50);
    tcsWriter.write(CODE_DOOR_OPENER);
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
        g_commandToSend = CODE_ENTRY_DOOR_BELL;
        g_shouldSend = true;
    }
    else if (strcmp(topic, mqttApartmentBell.getCommandTopic()) == 0)
    {
        g_commandToSend = CODE_APT_DOOR_BELL;
        g_shouldSend = true;
    }
    else if (strcmp(topic, mqttEntryOpener.getCommandTopic()) == 0)
    {
        g_commandToSend = CODE_DOOR_OPENER;
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
    

    pinMode(LED_BUILTIN, OUTPUT);
    // turn on led until boot sequence finished
    blinkLedAsync();

    Serial.begin(115200);
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

    client.setBufferSize(512);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

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

        if (cmd == CODE_APT_DOOR_BELL)
        {
            publishOnOffEdgeSwitch(&mqttApartmentBell);
        }

        if (cmd == CODE_ENTRY_DOOR_BELL)
        {
            publishOnOffEdgeSwitch(&mqttEntryBell);
        }

        if (cmd == CODE_DOOR_OPENER)
        {
            publishOnOffEdgeSwitch(&mqttEntryOpener);
        }

        if (g_partyMode && cmd == CODE_PARTY_MODE)
        {
            // we have a party, let everybody in
            openDoor();
        }

        if(cmd == CODE_HANDSET_LIFTUP)
        {
            if(millis() - g_tsLastHandsetLiftup < 2000)
            {
                g_handsetLiftup++;
            }
            else
            {
                g_handsetLiftup = 1;
            }
            g_tsLastHandsetLiftup = millis();

            if(g_handsetLiftup == 3)
            {
                g_partyMode = !g_partyMode;
                g_handsetLiftup = 0;
                blinkLedAsync();
                publishPartyMode();
            }
        }

        if (cmd == CODE_ENTRY_PATTERN_DETECT)
        {
            if (patternRecognitionEntry.trigger())
            {
                openDoor();
                publishOnOffEdgeBinary(&mqttEntryBellPattern);
            }
        }

        if (cmd == CODE_APARTMENT_PATTERN_DETECT)
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
