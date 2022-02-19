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

WiFiClient net;
PubSubClient client(net);

char wifi_ssid[] = "iot";        // your network SSID (name)
char wifi_pass[] = "iotdev1337"; // your network password (use for WPA, or use as key for WEP)

// define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_topic[60] = "home/flur/klingel/monitor";
char mqtt_topic_pattern[60] = "home/flur/klingel/pattern";
char mqtt_command_topic[60] = "home/flur/klingel/commands";
// TODO: use mqtt messages according to standard for push switches

// commands
// 0x1100 door opener if the handset is not lifted up
// 0x1180 door opener if the handset is lifted up
// 0x1B8F9A41 own door bell at the flat door
// 0x0B8F9A80 own door bell at the main door

//const uint32_t CODE_DOOR_OPENER = 0x1100; // use door opener code here
const uint32_t CODE_DOOR_OPENER = 0x1100; // use door opener code here
const uint32_t CODE_PATTERN_DETECT = 0x0B8F9A80; // use your bell code here


#define PIN_BUS_READ D5
#define PIN_BUS_WRITE D0

TriggerPatternRecognition patternRecognition;
TCSBusWriter tcsWriter(PIN_BUS_WRITE);
TCSBusReader tcsReader(PIN_BUS_READ);

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
    client.subscribe(mqtt_command_topic, 0);
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

void sendPatternDetected()
{
    client.publish(mqtt_topic_pattern, "patternDetected");
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
    char temp[32];
    strncpy ( temp, (char*) payload, length);
    uint32_t data = (uint32_t)strtoul(temp, NULL, 16);
    
    tcsWriter.write(data);
    Serial.println(data);
}


void setup()
{
    Serial.begin(115200);
    tcsWriter.begin();
    tcsReader.begin();
    //attachInterrupt(digitalPinToInterrupt(PIN_BUS_READ), analyzeCMD, CHANGE);
    // read configuration from FS json

    // configure pattern detection
    patternRecognition.addStep(1000);
    patternRecognition.addStep(1000);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pass);

    connectToWifi();

    Serial.println();
    Serial.print("Connected to SSID: ");
    Serial.println(wifi_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    client.setServer("192.168.42.2", 1883);
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
    if (tcsReader.hasCommand())
    {
        uint32_t cmd = tcsReader.read();
        if(cmd == CODE_PATTERN_DETECT)
        {
            if(patternRecognition.trigger())
            {
                openDoor();
                sendPatternDetected();
            }
        }
        Serial.print("TCS Bus: 0x");
        printHEX(cmd);
        Serial.println();

        char byte_cmd[9];
        sprintf(byte_cmd, "%08x", cmd);
        client.publish(mqtt_topic, byte_cmd);
    }
}
