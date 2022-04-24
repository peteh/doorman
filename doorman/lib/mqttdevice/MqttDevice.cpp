#include "MqttDevice.h"


String MqttEntity::getHomeAssistantConfigPayload()
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

    // add device config
    JsonObject device = doc.createNestedObject("device");
    device["identifiers"][0] = m_device->getIdentifier();
    device["name"] = m_device->getName();
    device["model"] = m_device->getModel();
    device["manufacturer"] = m_device->getManufacturer();

    String configData;
    serializeJson(doc, configData);
    return configData;
}