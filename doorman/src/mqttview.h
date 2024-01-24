#include <Arduino.h>
#include <PubSubClient.h>
#include <MqttDevice.h>
#include "platform.h"
#include "datastruct.h"
#include "utils.h"

class MqttView
{
public:
    MqttView(PubSubClient &client)
        : m_client(client)
    {
        MqttDevice mqttDevice(composeClientID().c_str(), "Doorman", SYSTEM_NAME, "maker_pt");

    }

private:
    PubSubClient m_client;

    //MqttDevice m_mqttDevice;
};