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

#include "utils.h"
#include "mqtttopics.h"

WiFiClient net;
PubSubClient client(net);

char wifi_ssid[] = "iot";        // your network SSID (name)
char wifi_pass[] = "iotdev1337"; // your network password (use for WPA, or use as key for WEP)

// mqtt server
char mqtt_server[255] = "192.168.2.50";
uint16_t mqtt_port = 1883;
char mqtt_user[60] = "";
char mqtt_pass[60] = "";

const char *HOMEASSISTANT_STATUS_TOPIC = "homeassistant/status";
const char *HOMEASSISTANT_STATUS_TOPIC_ALT = "ha/status";

// apartement door:
//   doorman-[name]/apartment/bell/state -> on/off
//   doorman-[name]/apartment/bell/pattern/state -> on/off
// entry door:
//   doorman-[name]/entry/bell/state -> on/off
//   doorman-[name]/entry/bell/pattern/state -> on/off
//   doorman-[name]/entry/opener/cmd
//   doorman-[name]/entry/autoopen/state

// commands
// 0x1100 door opener if the handset is not lifted up
// 0x1180 door opener if the handset is lifted up

// 0x1B8F9A41 own door bell at the flat door
// 0x0B8F9A80 own door bell at the main door

const uint32_t CODE_DOOR_OPENER = 0x1100;                  // use door opener code here (was 0x1100 for mine)
const uint32_t CODE_DOOR_OPENER_HANDSET_LIFTUP = 0x1180;   // use door opener code here (was 0x1100 for mine)
const uint32_t CODE_ENTRY_PATTERN_DETECT = 0x0B8F9A80;     // code to detect the pattern on, probably use your main door bell code here
const uint32_t CODE_APARTMENT_PATTERN_DETECT = 0x1B8F9A41; // code to detect the pattern on, probably use your main door bell code here
const uint32_t CODE_PARTY_MODE = 0x0B8F9A80;               // code that we react on to immidiately open the door (e.g. your main door bell or light switch)

const uint32_t CODE_APT_DOOR_BELL = 0x1B8F9A41;   // apartment door
const uint32_t CODE_ENTRY_DOOR_BELL = 0x0B8F9A80; // front door

bool partyMode = false;

#define PIN_BUS_READ D5
#define PIN_BUS_WRITE D6

TriggerPatternRecognition patternRecognitionEntry;
TriggerPatternRecognition patternRecognitionApartment;
TCSBusWriter tcsWriter(PIN_BUS_WRITE);
TCSBusReader tcsReader(PIN_BUS_READ);

uint32_t commandToSend = 0;
bool shouldSend = false;

bool ledState = false;
unsigned long tsLastLedStateOn = 0;

// TODO: factor out auto configuration into lib
// TODO: cleanup code
// TODO: initially send switch and sensor states after going online
// TODO: add party mode switch
// TODO: wifi auto config

void blinkLedAsync()
{
    digitalWrite(LED_BUILTIN, LOW);
    tsLastLedStateOn = millis();
    ledState = true;
}

void publishSwitchConfig(String device, String identifier, String name, String mainTopic)
{
    DynamicJsonDocument doc(2048);
    doc["name"] = name;
    doc["~"] = mainTopic;
    doc["command_topic"] = "~/cmd";
    doc["state_topic"] = "~/state";
    doc["payload_on"] = "on";
    doc["payload_off"] = "off";
    String configData;
    serializeJson(doc, configData);
    String configTopic = "homeassistant/switch/" + composeClientID() + "/" + identifier + "/config";
    client.publish(configTopic.c_str(), configData.c_str());
}

void publishBinarySensorConfig(String device, String identifier, String name, String mainTopic)
{
    DynamicJsonDocument doc(2048);
    doc["name"] = name;
    doc["~"] = mainTopic;
    doc["state_topic"] = "~/state";
    doc["payload_on"] = "on";
    doc["payload_off"] = "off";
    String configData;
    serializeJson(doc, configData);
    String configTopic = "homeassistant/binary_sensor/" + composeClientID() + "/" + identifier + "/config";
    client.publish(configTopic.c_str(), configData.c_str());
}

void publishConfig()
{
    publishSwitchConfig(composeClientID(), "entrybell", "Entry Bell", mqttTopic(MQTT_TOPIC_ENTRY_DOOR_BELL, MQTT_ACTION_NONE));
    publishSwitchConfig(composeClientID(), "apartmentbell", "Apartment Bell", mqttTopic(MQTT_TOPIC_APARTMENT_DOOR_BELL, MQTT_ACTION_NONE));
    publishSwitchConfig(composeClientID(), "dooropener", "Door Opener", mqttTopic(MQTT_TOPIC_ENTRY_DOOR_OPENER, MQTT_ACTION_NONE));
    publishSwitchConfig(composeClientID(), "dooropenerpartymode", "Door Opener Party Mode", mqttTopic(MQTT_TOPIC_PARTY_MODE, MQTT_ACTION_NONE));

    publishBinarySensorConfig(composeClientID(), "entrybellpattern", "Entry Bell Pattern", mqttTopic(MQTT_TOPIC_ENTRY_DOOR_BELL_PATTERN, MQTT_ACTION_NONE));
    publishBinarySensorConfig(composeClientID(), "apartmentbellpattern", "Apartment Bell Pattern", mqttTopic(MQTT_TOPIC_APARTMENT_DOOR_BELL_PATTERN, MQTT_ACTION_NONE));
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
    client.subscribe(mqttTopic(MQTT_TOPIC_BUS, MQTT_ACTION_CMD).c_str(), 1);
    client.subscribe(mqttTopic(MQTT_TOPIC_ENTRY_DOOR_BELL, MQTT_ACTION_CMD).c_str(), 1);
    client.subscribe(mqttTopic(MQTT_TOPIC_APARTMENT_DOOR_BELL, MQTT_ACTION_CMD).c_str(), 1);
    client.subscribe(mqttTopic(MQTT_TOPIC_ENTRY_DOOR_OPENER, MQTT_ACTION_CMD).c_str(), 1);
    client.subscribe(mqttTopic(MQTT_TOPIC_PARTY_MODE, MQTT_ACTION_CMD).c_str(), 1);

    client.publish(mqttTopic(MQTT_TOPIC_NONE, MQTT_ACTION_NONE).c_str(), "online");
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

void publishApartmentDoorBellPatternDetected()
{
    String topic = mqttTopic(MQTT_TOPIC_APARTMENT_DOOR_BELL_PATTERN, MQTT_ACTION_STATE);
    client.publish(topic.c_str(), "on");
    client.publish(topic.c_str(), "off");
}

void publishApartmentDoorBell()
{
    String topic = mqttTopic(MQTT_TOPIC_APARTMENT_DOOR_BELL, MQTT_ACTION_STATE);
    client.publish(topic.c_str(), "on");
    client.publish(topic.c_str(), "off");
}

void publishEntryDoorBellPatternDetected()
{
    String topic = mqttTopic(MQTT_TOPIC_ENTRY_DOOR_BELL_PATTERN, MQTT_ACTION_STATE);
    client.publish(topic.c_str(), "on");
    client.publish(topic.c_str(), "off");
}

void publishEntryDoorBell()
{
    String topic = mqttTopic(MQTT_TOPIC_ENTRY_DOOR_BELL, MQTT_ACTION_STATE);
    client.publish(topic.c_str(), "on");
    client.publish(topic.c_str(), "off");
}

void publishDoorOpener()
{
    String topic = mqttTopic(MQTT_TOPIC_ENTRY_DOOR_OPENER, MQTT_ACTION_STATE);
    client.publish(topic.c_str(), "on");
    client.publish(topic.c_str(), "off");
}

void publishPartyMode()
{
    String topic = mqttTopic(MQTT_TOPIC_PARTY_MODE, MQTT_ACTION_STATE);
    client.publish(topic.c_str(), partyMode ? "on" : "off");
}

void openDoor()
{
    delay(1000);
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

    if (strcmp(topic, mqttTopic(MQTT_TOPIC_BUS, MQTT_ACTION_CMD).c_str()) == 0)
    {
        char temp[32];
        strncpy(temp, (char *)payload, length);
        uint32_t data = (uint32_t)strtoul(temp, NULL, 16);
        commandToSend = data;
        shouldSend = true;
    }
    else if (strcmp(topic, mqttTopic(MQTT_TOPIC_ENTRY_DOOR_BELL, MQTT_ACTION_CMD).c_str()) == 0)
    {
        commandToSend = CODE_ENTRY_DOOR_BELL;
        shouldSend = true;
    }
    else if (strcmp(topic, mqttTopic(MQTT_TOPIC_APARTMENT_DOOR_BELL, MQTT_ACTION_CMD).c_str()) == 0)
    {
        commandToSend = CODE_APT_DOOR_BELL;
        shouldSend = true;
    }
    else if (strcmp(topic, mqttTopic(MQTT_TOPIC_ENTRY_DOOR_OPENER, MQTT_ACTION_CMD).c_str()) == 0)
    {
        commandToSend = CODE_DOOR_OPENER;
        shouldSend = true;
    }

    else if (strcmp(topic, mqttTopic(MQTT_TOPIC_PARTY_MODE, MQTT_ACTION_CMD).c_str()) == 0)
    {
        partyMode = strncmp((char *)payload, "on", length) == 0;
        publishPartyMode();
    }

    // register when homeassistant goes offline and needs the configuration again
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
    if (ledState)
    {
        if (millis() - tsLastLedStateOn > 50)
        {
            ledState = false;
            digitalWrite(LED_BUILTIN, HIGH);
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
    if (shouldSend)
    {
        uint32_t cmd = commandToSend;
        shouldSend = false;
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
            publishApartmentDoorBell();
        }

        if (cmd == CODE_ENTRY_DOOR_BELL)
        {
            publishEntryDoorBell();
        }

        if (cmd == CODE_DOOR_OPENER || cmd == CODE_DOOR_OPENER_HANDSET_LIFTUP)
        {
            publishDoorOpener();
        }

        if (partyMode && cmd == CODE_PARTY_MODE)
        {
            // we have a party, let everybody in
            openDoor();
        }

        if (cmd == CODE_ENTRY_PATTERN_DETECT)
        {
            if (patternRecognitionEntry.trigger())
            {
                openDoor();
                publishEntryDoorBellPatternDetected();
            }
        }

        if (cmd == CODE_APARTMENT_PATTERN_DETECT)
        {
            if (patternRecognitionApartment.trigger())
            {
                publishApartmentDoorBellPatternDetected();
            }
        }

        Serial.print("TCS Bus: 0x");
        printHEX(cmd);
        Serial.println();

        char byte_cmd[9];
        sprintf(byte_cmd, "%08x", cmd);
        client.publish(mqttTopic(MQTT_TOPIC_BUS, MQTT_ACTION_STATE).c_str(), byte_cmd);
    }
}
