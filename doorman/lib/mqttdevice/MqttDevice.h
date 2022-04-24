#include <Arduino.h>
#include <ArduinoJson.h>

class MqttDevice
{
public:
    MqttDevice(const char *identifier, const char *name, const char *model)
    {
        strncpy(m_identifier, identifier, sizeof(m_identifier));
        strncpy(m_name, name, sizeof(m_name));
        strncpy(m_model, model, sizeof(m_model));
    }

public:
    const char *getIdentifier() const
    {
        return m_identifier;
    }

    const char *getName() const
    {
        return m_name;
    }

    const char *getModel() const
    {
        return m_model;
    }

private:
    char m_identifier[64];
    char m_name[64];
    char m_model[64];
};

class MqttEntity
{
public:
    MqttEntity(const MqttDevice *mqttDevice, const char *nodeId, const char *objectId, const char *type, const char *humanName, const char *subTopic)
    {
        m_device = mqttDevice;
        strncpy(m_nodeId, nodeId, sizeof(m_nodeId));
        strncpy(m_objectId, objectId, sizeof(m_objectId));
        strncpy(m_type, type, sizeof(m_type));
        strncpy(m_humanName, humanName, sizeof(m_humanName));
        strncpy(m_subTopic, subTopic, sizeof(m_subTopic));

        snprintf(m_cmdTopic, sizeof(m_cmdTopic), "%s/%s/%s", m_nodeId, m_subTopic, m_cmdSubTopic);
        snprintf(m_stateTopic, sizeof(m_cmdTopic), "%s/%s/%s", m_nodeId, m_subTopic, m_stateSubTopic);
        snprintf(m_uniqueId, sizeof(m_uniqueId), "%s-%s", m_nodeId, m_objectId);
    }

    void setHasCommandTopic(bool hasCommand)
    {
        m_hasCommandTopic = true;
    }

    void getBaseTopic(char *baseTopic_, size_t bufferSize)
    {
        snprintf(baseTopic_, bufferSize, "%s/%s", m_nodeId, m_subTopic);
    }

    const char *getCommandTopic()
    {
        return m_cmdTopic;
    }

    void getCommandTopic(char *commandTopic_, size_t bufferSize)
    {
        snprintf(commandTopic_, bufferSize, "%s/%s/%s", m_nodeId, m_subTopic, m_cmdSubTopic);
    }

    const char *getStateTopic()
    {
        return m_stateTopic;
    }

    void getStateTopic(char *stateTopic_, size_t bufferSize)
    {
        snprintf(stateTopic_, bufferSize, "%s/%s/%s", m_nodeId, m_subTopic, m_stateSubTopic);
    }

    const char *getHumanName()
    {
        return m_humanName;
    }

    void getHomeAssistantConfigTopic(char *configTopic_, size_t bufferSize)
    {
        // TODO:  homeassistant/[type]/[device identifier]/[object id]/config
        snprintf(configTopic_, bufferSize, "homeassistant/%s/%s/config",
                 m_type,
                 m_uniqueId);
    }

    void getHomeAssistantConfigTopicAlt(char *configTopic_, size_t bufferSize)
    {
        snprintf(configTopic_, bufferSize, "ha/%s/%s/config",
                 m_type,
                 m_uniqueId);
    }

    String getHomeAssistantConfigPayload();

    String getOnlineState();

protected:
    virtual void addConfig(DynamicJsonDocument &doc) = 0;

private:
    const MqttDevice *m_device; // the device this entity belongs to
    char m_nodeId[32];    // our node that publishes the different devices, e.g. doorman-3434
    char m_objectId[32];  // our actual device identifier, e.g. doorbell, must be unique within the nodeid
    char m_type[16];      // mqtt device type, e.g. switch
    char m_uniqueId[64];  // the unique identifier, e.g. doorman2323-doorbell
    char m_humanName[64]; // human readbable name, e.g. Door Bell

    char m_subTopic[128]; // topic under nodeid/x/y, e.g. door/bell

    bool m_hasCommandTopic = false;
    char m_cmdTopic[255];
    char m_stateTopic[255];

    const char *m_cmdSubTopic = "cmd";
    const char *m_stateSubTopic = "state";

    const char *m_stateOnline = "online";
};

class MqttBinarySensor : public MqttEntity
{
public:
    MqttBinarySensor(MqttDevice *device, const char *nodeId, const char *objectId, const char *humanName, const char *subTopic)
        : MqttEntity(device, nodeId, objectId, "binary_sensor", humanName, subTopic)
    {
    }

    const char *getOnState()
    {
        return m_stateOn;
    }

    const char *getOffState()
    {
        return m_stateOff;
    }

protected:
    virtual void addConfig(DynamicJsonDocument &doc)
    {
        doc["payload_on"] = m_stateOn;
        doc["payload_off"] = m_stateOff;
    }

private:
    const char *m_stateOn = "on";
    const char *m_stateOff = "off";
};

class MqttSwitch : public MqttEntity
{
public:
    MqttSwitch(MqttDevice *device, const char *nodeId, const char *objectId, const char *humanName, const char *subTopic)
        : MqttEntity(device, nodeId, objectId, "switch", humanName, subTopic)
    {
        setHasCommandTopic(true);
    }

    const char *getOnState()
    {
        return m_stateOn;
    }

    const char *getOffState()
    {
        return m_stateOff;
    }

protected:
    virtual void addConfig(DynamicJsonDocument &doc)
    {
        Serial.print("Adding config for Switch");
        doc["state_on"] = m_stateOn;
        doc["state_off"] = m_stateOff;
        doc["payload_on"] = m_stateOn;
        doc["payload_off"] = m_stateOff;
    }

private:
    const char *m_stateOn = "on";
    const char *m_stateOff = "off";
};

class MqttLock : public MqttEntity
{
public:
    MqttLock(MqttDevice *device, const char *nodeId, const char *objectId, const char *humanName, const char *subTopic)
        : MqttEntity(device, nodeId, objectId, "lock", humanName, subTopic)
    {
        setHasCommandTopic(true);
    }

    const char *getLockCommand()
    {
        return m_cmdLock;
    }

    const char *getUnlockCommand()
    {
        return m_cmdUnlock;
    }

    const char *getOpenCommand()
    {
        return m_cmdOpen;
    }

    const char *getLockedState()
    {
        return m_stateLocked;
    }

    const char *getUnlockedState()
    {
        return m_stateUnlocked;
    }

protected:
    virtual void addConfig(DynamicJsonDocument &doc)
    {
        Serial.print("Adding config for Lock");
        doc["payload_lock"] = m_cmdLock;
        doc["payload_unlock"] = m_cmdUnlock;
        doc["payload_open"] = m_cmdOpen;
        doc["state_locked"] = m_stateLocked;
        doc["state_unlocked"] = m_stateUnlocked;
    }

private:
    const char *m_cmdLock = "lock";
    const char *m_cmdUnlock = "unlock";
    const char *m_cmdOpen = "open";
    const char *m_stateLocked = "locked";
    const char *m_stateUnlocked = "unlocked";
};
