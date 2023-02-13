#include <Arduino.h>
#include <ArduinoJson.h>


enum EntityCategory { NONE, DIAGNOSTIC, CONFIG };

class MqttDevice
{
public:
    MqttDevice(const char *identifier, const char *name, const char *model, const char *manufacturer)
        : m_configurationUrl("")
    {
        strncpy(m_identifier, identifier, sizeof(m_identifier));
        strncpy(m_name, name, sizeof(m_name));
        strncpy(m_model, model, sizeof(m_model));
        strncpy(m_manufacturer, manufacturer, sizeof(m_manufacturer));
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

    const char *getManufacturer() const
    {
        return m_manufacturer;
    }

    void setConfigurationUrl(char *configurationUrl)
    {
        strncpy(m_configurationUrl, configurationUrl, sizeof(m_configurationUrl));
    }

    void addConfig(DynamicJsonDocument &doc) const
    {
        JsonObject device = doc.createNestedObject("device");
        device["identifiers"][0] = getIdentifier();
        device["name"] = getName();
        device["model"] = getModel();
        device["manufacturer"] = getManufacturer();
        if (strlen(m_configurationUrl) > 0)
        {
            device["configuration_url"] = m_configurationUrl;
        }
    }

private:
    char m_identifier[64];
    char m_name[64];
    char m_model[64];
    char m_manufacturer[64];
    char m_configurationUrl[256];
};

class MqttEntity
{
public:
    MqttEntity(const MqttDevice *mqttDevice, const char *objectId, const char *type, const char *humanName)
    {
        m_device = mqttDevice;
        strncpy(m_objectId, objectId, sizeof(m_objectId));
        strncpy(m_type, type, sizeof(m_type));
        strncpy(m_humanName, humanName, sizeof(m_humanName));

        snprintf(m_cmdTopic, sizeof(m_cmdTopic), "%s/%s/%s", m_device->getIdentifier(), m_objectId, m_cmdSubTopic);
        snprintf(m_stateTopic, sizeof(m_cmdTopic), "%s/%s/%s", m_device->getIdentifier(), m_objectId, m_stateSubTopic);
        snprintf(m_uniqueId, sizeof(m_uniqueId), "%s-%s", m_device->getIdentifier(), m_objectId);
    }

    void setHasCommandTopic(bool hasCommand)
    {
        m_hasCommandTopic = hasCommand;
    }

    void getBaseTopic(char *baseTopic_, size_t bufferSize)
    {
        snprintf(baseTopic_, bufferSize, "%s/%s", m_device->getIdentifier(), m_objectId);
    }

    const char *getCommandTopic()
    {
        return m_cmdTopic;
    }

    void getCommandTopic(char *commandTopic_, size_t bufferSize)
    {
        snprintf(commandTopic_, bufferSize, "%s/%s/%s", m_device->getIdentifier(), m_objectId, m_cmdSubTopic);
    }

    const char *getStateTopic()
    {
        return m_stateTopic;
    }

    void setCustomStateTopic(const char *customStateTopic)
    {
        strncpy(m_stateTopic, customStateTopic, sizeof(m_stateTopic));
    }

    void setValueTemplate(const char *valueTemplate)
    {
        strncpy(m_valueTemplate, valueTemplate, sizeof(m_valueTemplate));
    }

    void setUnit(const char *unit)
    {
        strncpy(m_unit, unit, sizeof(m_unit));
    }

    void setDeviceClass(const char *deviceClass)
    {
        strncpy(m_deviceClass, deviceClass, sizeof(m_deviceClass));
    }

    void setIcon(const char *icon)
    {
        strncpy(m_icon, icon, sizeof(m_icon));
    }

    void setEntityType(EntityCategory type)
    {
        m_entityType = type;
    }

    const char *getHumanName()
    {
        return m_humanName;
    }

    void getHomeAssistantConfigTopic(char *configTopic_, size_t bufferSize)
    {
        // TODO:  homeassistant/[type]/[device identifier]/[object id]/config
        snprintf(configTopic_, bufferSize, "homeassistant/%s/%s/%s/config",
                 m_type,
                 m_device->getIdentifier(),
                 m_objectId);
    }

    void getHomeAssistantConfigTopicAlt(char *configTopic_, size_t bufferSize)
    {
        snprintf(configTopic_, bufferSize, "ha/%s/%s/%s/config",
                 m_type,
                 m_device->getIdentifier(),
                 m_objectId);
    }

    String getHomeAssistantConfigPayload();

    String getOnlineState();

protected:
    virtual void addConfig(DynamicJsonDocument &doc) = 0;

    const MqttDevice *getDevice()
    {
        return m_device;
    }

    const char *getObjectId()
    {
        return m_objectId;
    }

private:
    const MqttDevice *m_device; // the device this entity belongs to
    char m_objectId[32];        // our actual device identifier, e.g. doorbell, must be unique within the nodeid
    char m_type[16];            // mqtt device type, e.g. switch
    char m_uniqueId[96];        // the unique identifier, e.g. doorman2323-doorbell
    char m_humanName[64];       // human readbable name, e.g. Door Bell

    bool m_hasCommandTopic = false;
    char m_cmdTopic[255] = "";
    char m_stateTopic[255] = "";
    char m_valueTemplate[255] = "";
    char m_unit[10] = "";
    char m_deviceClass[32] = "";
    char m_icon[128] = "";

    EntityCategory m_entityType = EntityCategory::NONE;

    const char *m_cmdSubTopic = "cmd";
    const char *m_stateSubTopic = "state";

    const char *m_stateOnline = "online";
};

class MqttBinarySensor : public MqttEntity
{
public:
    MqttBinarySensor(MqttDevice *device, const char *objectId, const char *humanName)
        : MqttEntity(device, objectId, "binary_sensor", humanName)
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

class MqttSensor : public MqttEntity
{
public:
    MqttSensor(MqttDevice *device, const char *objectId, const char *humanName)
        : MqttEntity(device, objectId, "sensor", humanName)
    {
    }

protected:
    virtual void addConfig(DynamicJsonDocument &doc)
    {
        // doc["payload_on"] = m_stateOn;
        // doc["payload_off"] = m_stateOff;
    }
};

class MqttSwitch : public MqttEntity
{
public:
    MqttSwitch(MqttDevice *device, const char *objectId, const char *humanName)
        : MqttEntity(device, objectId, "switch", humanName)
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

class MqttText : public MqttEntity
{
public:
    MqttText(MqttDevice *device, const char *objectId, const char *humanName)
        : MqttEntity(device, objectId, "text", humanName)
    {
        setHasCommandTopic(true);
    }

    void setPattern(const char *pattern)
    {
        strncpy(m_pattern, pattern, sizeof(m_pattern));
    }

    void setMinLetters(int16_t minLetters)
    {
        m_min = minLetters;
    }

    void setMaxLetters(int16_t maxLetters)
    {
        m_max = maxLetters;
    }

protected:
    virtual void addConfig(DynamicJsonDocument &doc)
    {
        Serial.print("Adding config for Text");
        if (strlen(m_pattern) > 0)
        {
            doc["pattern"] = m_pattern;
        }
        if (m_min >= 0)
        {
            doc["min"] = m_min;
        }
        if (m_max >= 0)
        {
            doc["max"] = m_max;
        }
    }

private:
    char m_pattern[255] = "";
    int16_t m_min = -1;
    int16_t m_max = -1;
};

class MqttLock : public MqttEntity
{
public:
    MqttLock(MqttDevice *device, const char *objectId, const char *humanName)
        : MqttEntity(device, objectId, "lock", humanName)
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

class MqttCover : public MqttEntity
{
public:
    MqttCover(MqttDevice *device, const char *objectId, const char *humanName)
        : MqttEntity(device, objectId, "cover", humanName)
    {
        setHasCommandTopic(true);
        snprintf(m_cmdPositionTopic, sizeof(m_cmdPositionTopic), "%s/%s/%s", getDevice()->getIdentifier(), getObjectId(), m_cmdPositionSubTopic);
        snprintf(m_positionTopic, sizeof(m_positionTopic), "%s/%s/%s", getDevice()->getIdentifier(), getObjectId(), m_positionSubTopic);
    }

    const char *getCommandPositionTopic()
    {
        return m_cmdPositionTopic;
    }

    const char *getPositionTopic()
    {
        return m_positionTopic;
    }

    const char *getOpenCommand()
    {
        return m_cmdOpen;
    }

    const char *getCloseCommand()
    {
        return m_cmdClose;
    }

    const char *getStopCommand()
    {
        return m_cmdStop;
    }

    const char *getOpeningState()
    {
        return m_stateOpening;
    }

    const char *getClosingState()
    {
        return m_stateClosing;
    }

    const char *getStoppedState()
    {
        return m_stateStopped;
    }

protected:
    virtual void addConfig(DynamicJsonDocument &doc)
    {
        Serial.print("Adding config for Cover");
        doc["payload_open"] = m_cmdOpen;
        doc["payload_close"] = m_cmdClose;
        doc["payload_stop"] = m_cmdStop;

        doc["state_opening"] = m_stateOpening;
        doc["state_stopped"] = m_stateStopped;
        doc["state_closing"] = m_stateClosing;

        doc["set_position_topic"] = m_cmdPositionTopic;
        doc["position_topic"] = m_positionTopic;
        doc["position_open"] = m_positionOpen;
        doc["position_closed"] = m_positionClosed;
    }

private:
    const char *m_cmdOpen = "open";
    const char *m_cmdClose = "close";
    const char *m_cmdStop = "stop";

    const char *m_stateOpening = "opening";
    const char *m_stateClosing = "closing";
    const char *m_stateStopped = "stopped";

    const uint8_t m_positionOpen = 100;
    const uint8_t m_positionClosed = 0;

    const char *m_cmdPositionSubTopic = "cmd_position";
    const char *m_positionSubTopic = "position";

    char m_cmdPositionTopic[255];
    char m_positionTopic[255];
};
