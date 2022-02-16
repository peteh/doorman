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

#include <ESP8266Ping.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

WiFiClient net;
PubSubClient client(net);

char wifi_ssid[] = "iot";        // your network SSID (name)
char wifi_pass[] = "iotdev1337"; // your network password (use for WPA, or use as key for WEP)

// define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_topic[34] = "home/flur/klingel";
char mqtt_client[16] = "klingel";


// flag for saving data
bool shouldSaveConfig = false;

#define inputPin 14 // D5
#define startBit 6
#define oneBit 4
#define zeroBit 2

volatile uint32_t CMD = 0;
volatile uint8_t lengthCMD = 0;
volatile bool cmdReady;

void connectToMqtt()
{
    Serial.print("\nconnecting to MQTT...");
    // while (!client.connect(mqtt_client, mqtt_user, mqtt_pass))
    while (!client.connect("mqttclientid"))
    {
        Serial.print(".");
        delay(5000);
    }
}

void connectToWifi()
{
    Serial.print("checking wifi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }

    IPAddress ip(192, 168, 42, 3); // The remote ip to ping

    connectToMqtt();

    Serial.println("\nconnected!");
}

void sendMessage()
{
    // connectToMqtt();
    client.publish(mqtt_topic, "triggered");
    // delay(100);
    // client.disconnect();
}

// callback notifying us of the need to save config
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void ICACHE_RAM_ATTR analyzeCMD()
{
    static uint32_t curCMD;
    static uint32_t usLast;
    static byte curCRC;
    static byte calCRC;
    static byte curLength;
    static byte cmdIntReady;
    static byte curPos;
    uint32_t usNow = micros();
    uint32_t timeInUS = usNow - usLast;
    usLast = usNow;
    byte curBit = 4;
    if (timeInUS >= 1000 && timeInUS <= 2999)
    {
        curBit = 0;
    }
    else if (timeInUS >= 3000 && timeInUS <= 4999)
    {
        curBit = 1;
    }
    else if (timeInUS >= 5000 && timeInUS <= 6999)
    {
        curBit = 2;
    }
    else if (timeInUS >= 7000 && timeInUS <= 24000)
    {
        curBit = 3;
        curPos = 0;
    }

    if (curPos == 0)
    {
        if (curBit == 2)
        {
            curPos++;
        }
        curCMD = 0;
        curCRC = 0;
        calCRC = 1;
        curLength = 0;
    }
    else if (curBit == 0 || curBit == 1)
    {
        if (curPos == 1)
        {
            curLength = curBit;
            curPos++;
        }
        else if (curPos >= 2 && curPos <= 17)
        {
            if (curBit)
                bitSet(curCMD, (curLength ? 33 : 17) - curPos);
            calCRC ^= curBit;
            curPos++;
        }
        else if (curPos == 18)
        {
            if (curLength)
            {
                if (curBit)
                    bitSet(curCMD, 33 - curPos);
                calCRC ^= curBit;
                curPos++;
            }
            else
            {
                curCRC = curBit;
                cmdIntReady = 1;
            }
        }
        else if (curPos >= 19 && curPos <= 33)
        {
            if (curBit)
                bitSet(curCMD, 33 - curPos);
            calCRC ^= curBit;
            curPos++;
        }
        else if (curPos == 34)
        {
            curCRC = curBit;
            cmdIntReady = 1;
        }
    }
    else
    {
        curPos = 0;
    }
    if (cmdIntReady)
    {
        cmdIntReady = 0;
        if (curCRC == calCRC)
        {
            cmdReady = 1;
            lengthCMD = curLength;
            CMD = curCMD;
        }
        curCMD = 0;
        curPos = 0;
    }
}

void printHEX(uint32_t DATA)
{
    uint8_t numChars = lengthCMD ? 8 : 4;
    uint32_t mask = 0x0000000F;
    mask = mask << 4 * (numChars - 1);
    for (uint32_t i = numChars; i > 0; --i)
    {
        Serial.print(((DATA & mask) >> (i - 1) * 4), HEX);
        mask = mask >> 4;
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(inputPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(inputPin), analyzeCMD, CHANGE);
    // read configuration from FS json

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pass);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Node7b: Connected to ");
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

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
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
    }
  });
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
    if (cmdReady)
    {
        cmdReady = 0;
        Serial.write(0x01);
        Serial.print("$");
        printHEX(CMD);
        Serial.write(0x04);
        Serial.println();

        char byte_cmd[9];
        sprintf(byte_cmd, "%08x", CMD);
        client.publish(mqtt_topic, byte_cmd);
    }
}
