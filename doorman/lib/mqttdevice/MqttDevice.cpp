#include "MqttDevice.h"


String MqttDevice::getHomeAssistantConfigPayload()
{
    DynamicJsonDocument doc(2048);

    doc["name"] = m_humanName;

    doc["unique_id"] = m_uniqueId;

    char baseTopic[255];
    getBaseTopic(baseTopic, sizeof(baseTopic));
    doc["~"] = baseTopic;
    
    if (m_hasCommandTopic)
    {
        doc["command_topic"] = "~/cmd";
    }
    doc["state_topic"] = "~/state";

    // add the other configurations
    addConfig(doc);

    String configData;
    serializeJson(doc, configData);
    return configData;
}