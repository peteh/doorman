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
    //doc["state_topic"] = "~/state";
    doc["state_topic"] = m_stateTopic;

    // add the other configurations
    addConfig(doc);

    if(m_valueTemplate[0] != 0)
    {
        doc["value_template"] = m_valueTemplate;
    }

    if(m_unit[0] != 0)
    {
        doc["unit_of_measurement"] = m_unit;
    }

    if(m_deviceClass[0] != 0)
    {
        doc["device_class"] = m_deviceClass;
    }

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