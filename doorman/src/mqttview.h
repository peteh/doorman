#include <Arduino.h>
#include <PubSubClient.h>
#include <MqttDevice.h>
#include <esplog.h>
#include "platform.h"
#include "utils.h"
#include "settings.h"

class MqttView
{
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

    // 0x1B8F9A41 own door bell at the flat door
    // 0x0B8F9A80 own door bell at the main door

public:
    MqttView(PubSubClient *client)
        : m_client(client),
          m_device(composeClientID().c_str(), "Doorman", SYSTEM_NAME, "maker_pt"),
          m_apartmentBell(&m_device, "apartmentbell", "Apartment Bell"),
          m_apartmentBellPattern(&m_device, "apartmentbellpattern", "Apartment Bell Pattern"),

          m_entryBell(&m_device, "entrybell", "Entry Bell"),
          m_entryBellPattern(&m_device, "entrybellpattern", "Entry Bell Pattern"),
          m_entryOpener(&m_device, "entryopener", "Door Opener"),
          m_relay(&m_device, "relay", "Relay"),

          m_partyMode(&m_device, "partymode", "Party Mode"),

          m_bus(&m_device, "bus", "TCS Bus"),

          // Configuration Settings
          m_configCodeApartmentDoorBell(&m_device, "config_code_apartment_door_bell", "Apartment Door Bell Code"),
          m_configCodeEntryDoorBell(&m_device, "config_code_entry_door_bell", "Entry Door Bell Code"),
          m_configCodeHandsetLiftup(&m_device, "config_code_handset_liftup", "Handset Liftup Code"),
          m_configCodeDoorOpener(&m_device, "config_code_door_opener", "Entry Door Opener Code"),
          m_configCodeApartmentPatternDetect(&m_device, "config_code_apartment_pattern_detected", "Apartment Pattern Detected Code"),
          m_configCodeEntryPatternDetect(&m_device, "config_code_entry_pattern_detected", "Entry Pattern Detected Code"),
          m_configCodePartyMode(&m_device, "config_code_party_mode", "Party Mode Code"),

          // Diagnostics Elements
          m_diagnosticsResetButton(&m_device, "diagnostics_reset_btn", "Reset Counters"),
          m_diagnosticsRestartCounter(&m_device, "diagnostics_restart_counter", "Restart Counter"),
          m_diagnosticsWifiDisconnectCounter(&m_device, "diagnostics_wifidisconnect_counter", "WiFi Disconnect Counter"),
          m_diagnosticsMqttDisconnectCounter(&m_device, "diagnostics_mqttdisconnect_counter", "MQTT Disconnect Counter"),
          m_diagnosticsBssid(&m_device, "diagnostics_bssid", "BSSID")
    {

        m_device.setSWVersion(VERSION);

        // further mqtt device config
        m_bus.setPattern("[a-fA-F0-9]*");
        m_bus.setMaxLetters(8);
        m_bus.setIcon("mdi:console-network");

        m_partyMode.setIcon("mdi:door-closed-lock");
        m_entryOpener.setIcon("mdi:door-open");

        m_configCodeApartmentDoorBell.setPattern("[a-fA-F0-9]*");
        m_configCodeApartmentDoorBell.setMaxLetters(8);
        m_configCodeApartmentDoorBell.setEntityType(EntityCategory::CONFIG);

        m_configCodeEntryDoorBell.setPattern("[a-fA-F0-9]*");
        m_configCodeEntryDoorBell.setMaxLetters(8);
        m_configCodeEntryDoorBell.setEntityType(EntityCategory::CONFIG);

        m_configCodeHandsetLiftup.setPattern("[a-fA-F0-9]*");
        m_configCodeHandsetLiftup.setMaxLetters(8);
        m_configCodeHandsetLiftup.setEntityType(EntityCategory::CONFIG);

        m_configCodeDoorOpener.setPattern("[a-fA-F0-9]*");
        m_configCodeDoorOpener.setMaxLetters(8);
        m_configCodeDoorOpener.setEntityType(EntityCategory::CONFIG);

        m_configCodeApartmentPatternDetect.setPattern("[a-fA-F0-9]*");
        m_configCodeApartmentPatternDetect.setMaxLetters(8);
        m_configCodeApartmentPatternDetect.setEntityType(EntityCategory::CONFIG);

        m_configCodeEntryPatternDetect.setPattern("[a-fA-F0-9]*");
        m_configCodeEntryPatternDetect.setMaxLetters(8);
        m_configCodeEntryPatternDetect.setEntityType(EntityCategory::CONFIG);

        m_configCodePartyMode.setPattern("[a-fA-F0-9]*");
        m_configCodePartyMode.setMaxLetters(8);
        m_configCodePartyMode.setEntityType(EntityCategory::CONFIG);

        m_diagnosticsResetButton.setEntityType(EntityCategory::DIAGNOSTIC);
        m_diagnosticsResetButton.setDeviceClass("restart");

        m_diagnosticsRestartCounter.setEntityType(EntityCategory::DIAGNOSTIC);
        m_diagnosticsRestartCounter.setStateClass(MqttSensor::StateClass::TOTAL);
        m_diagnosticsRestartCounter.setIcon("mdi:counter");

        m_diagnosticsMqttDisconnectCounter.setEntityType(EntityCategory::DIAGNOSTIC);
        m_diagnosticsMqttDisconnectCounter.setStateClass(MqttSensor::StateClass::TOTAL);
        m_diagnosticsMqttDisconnectCounter.setIcon("mdi:counter");

        m_diagnosticsWifiDisconnectCounter.setEntityType(EntityCategory::DIAGNOSTIC);
        m_diagnosticsWifiDisconnectCounter.setStateClass(MqttSensor::StateClass::TOTAL);
        m_diagnosticsWifiDisconnectCounter.setIcon("mdi:counter");

        m_diagnosticsBssid.setEntityType(EntityCategory::DIAGNOSTIC);
        m_diagnosticsBssid.setIcon("mdi:wifi");
    }

    MqttDevice &getDevice()
    {
        return m_device;
    }

    const MqttSiren &getApartmentBell() const
    {
        return m_apartmentBell;
    }

    const MqttBinarySensor &getApartmentBellPattern() const
    {
        return m_apartmentBellPattern;
    }

    const MqttSiren &getEntryBell() const
    {
        return m_entryBell;
    }

    const MqttBinarySensor &getEntryBellPattern() const
    {
        return m_entryBellPattern;
    }

    const MqttSwitch &getEntryOpener() const
    {
        return m_entryOpener;
    }

    const MqttSwitch &getRelay() const
    {
        return m_relay;
    }

    const MqttSwitch &getPartyMode() const
    {
        return m_partyMode;
    }

    const MqttText &getBus() const
    {
        return m_bus;
    }

    const MqttText &getConfigCodeApartmentDoorBell() const
    {
        return m_configCodeApartmentDoorBell;
    }

    const MqttText &getConfigCodeEntryDoorBell() const
    {
        return m_configCodeEntryDoorBell;
    }

    const MqttText &getConfigCodeHandsetLiftup() const
    {
        return m_configCodeHandsetLiftup;
    }

    const MqttText &getConfigCodeDoorOpener() const
    {
        return m_configCodeDoorOpener;
    }

    const MqttText &getConfigCodeApartmentPatternDetect() const
    {
        return m_configCodeApartmentPatternDetect;
    }

    const MqttText &getConfigCodeEntryPatternDetect() const
    {
        return m_configCodeEntryPatternDetect;
    }

    const MqttText &getConfigCodePartyMode() const
    {
        return m_configCodePartyMode;
    }

    const MqttButton &getDiagnosticsResetButton() const
    {
        return m_diagnosticsResetButton;
    }

    const MqttSensor &getDiagnosticsRestartCounter() const
    {
        return m_diagnosticsRestartCounter;
    }

    const MqttSensor &getDiagnosticsWifiDisconnectCounter() const
    {
        return m_diagnosticsWifiDisconnectCounter;
    }

    const MqttSensor &getDiagnosticsMqttDisconnectCounter() const
    {
        return m_diagnosticsMqttDisconnectCounter;
    }

    void publishOnOffEdgeSwitch(const MqttSwitch &entity)
    {
        publishMqttState(entity, entity.getOnState());
        delay(1000);
        publishMqttState(entity, entity.getOffState());
    }

    void publishOnOffEdgeSiren(MqttSiren &entity)
    {
        publishMqttState(entity, entity.getOnState());
        delay(1000);
        publishMqttState(entity, entity.getOffState());
    }

    void publishOnOffEdgeBinary(MqttBinarySensor &entity)
    {
        publishMqttState(entity, entity.getOnState());
        delay(1000);
        publishMqttState(entity, entity.getOffState());
    }

    void publishConfig(Settings &settings)
    {
        publishConfig(m_apartmentBell);
        publishConfig(m_apartmentBellPattern);

        publishConfig(m_entryBell);
        publishConfig(m_entryBellPattern);
        publishConfig(m_entryOpener);
        publishConfig(m_relay);

        publishConfig(m_partyMode);

        publishConfig(m_bus);

        // config elements
        publishConfig(m_configCodeApartmentDoorBell);
        publishConfig(m_configCodeEntryDoorBell);
        publishConfig(m_configCodeHandsetLiftup);
        publishConfig(m_configCodeDoorOpener);
        publishConfig(m_configCodeApartmentPatternDetect);
        publishConfig(m_configCodeEntryPatternDetect);
        publishConfig(m_configCodePartyMode);

        // diag elements
        publishConfig(m_diagnosticsResetButton);
        publishConfig(m_diagnosticsRestartCounter);
        publishConfig(m_diagnosticsWifiDisconnectCounter);
        publishConfig(m_diagnosticsMqttDisconnectCounter);
        publishConfig(m_diagnosticsBssid);

        delay(1000);

        // publish all initial states
        publishMqttState(m_apartmentBell, m_apartmentBell.getOffState());
        publishMqttState(m_apartmentBellPattern, m_apartmentBellPattern.getOffState());

        publishMqttState(m_entryBell, m_entryBell.getOffState());
        publishMqttState(m_entryBellPattern, m_entryBellPattern.getOffState());
        publishMqttState(m_entryOpener, m_entryOpener.getOffState());
        publishMqttState(m_bus, "");
        publishPartyMode(settings.getGeneralSettings().partyMode);

        publishConfigValues(settings);

        publishDiagnostics(settings, "");
    }

    void publishConfigValues(Settings &settings)
    {
        publishMqttConfigState(m_configCodeApartmentDoorBell, settings.getCodeSettings().codeApartmentDoorBell);
        publishMqttConfigState(m_configCodeEntryDoorBell, settings.getCodeSettings().codeEntryDoorBell);
        publishMqttConfigState(m_configCodeHandsetLiftup, settings.getCodeSettings().codeHandsetLiftup);
        publishMqttConfigState(m_configCodeDoorOpener, settings.getCodeSettings().codeDoorOpener);
        publishMqttConfigState(m_configCodeApartmentPatternDetect, settings.getCodeSettings().codeApartmentPatternDetect);
        publishMqttConfigState(m_configCodeEntryPatternDetect, settings.getCodeSettings().codeEntryPatternDetect);
        publishMqttConfigState(m_configCodePartyMode, settings.getCodeSettings().codePartyMode);
    }

    void publishDiagnostics(Settings &settings, const char* bssid)
    {
        publishMqttCounterState(m_diagnosticsRestartCounter, settings.getGeneralSettings().restartCounter);
        publishMqttCounterState(m_diagnosticsWifiDisconnectCounter, settings.getGeneralSettings().wifiDisconnectCounter);
        publishMqttCounterState(m_diagnosticsMqttDisconnectCounter, settings.getGeneralSettings().mqttDisconnectCounter);
        publishMqttState(m_diagnosticsBssid, bssid);
    }

    void publishPartyMode(bool partyMode)
    {
        publishMqttState(m_partyMode, partyMode ? m_partyMode.getOnState() : m_partyMode.getOffState());
    }

    void publishEntryOpenerTrigger()
    {
        publishOnOffEdgeSwitch(m_entryOpener);
    }

    void publishEntryBellPatternTrigger()
    {
        publishOnOffEdgeBinary(m_entryBellPattern);
    }

    void publishApartmentBellPatternTrigger()
    {
        publishOnOffEdgeBinary(m_apartmentBellPattern);
    }

    void publishEntryBellTrigger()
    {
        publishOnOffEdgeSiren(m_entryBell);
    }

    void publishApartmentBellTrigger()
    {
        publishOnOffEdgeSiren(m_apartmentBell);
    }

    void publishBus(uint32_t cmd)
    {
        char byte_cmd[9];
        sprintf(byte_cmd, "%08x", cmd);
        if (!m_client->publish(m_bus.getStateTopic(), byte_cmd))
        {
            log_error("Failed to publish tcs data to %s", m_bus.getStateTopic());
        }
    }
    void publishMqttState(const MqttEntity &entity, const char *state)
    {
        if (!m_client->publish(entity.getStateTopic(), state))
        {
            log_error("Failed to publish state to %s", entity.getStateTopic());
        }
    }

private:
    PubSubClient *m_client;

    MqttDevice m_device;
    MqttSiren m_apartmentBell;
    MqttBinarySensor m_apartmentBellPattern;

    MqttSiren m_entryBell;
    MqttBinarySensor m_entryBellPattern;
    MqttSwitch m_entryOpener;
    MqttSwitch m_relay;
    MqttSwitch m_partyMode;

    MqttText m_bus;

    // Configuration Settings
    MqttText m_configCodeApartmentDoorBell;
    MqttText m_configCodeEntryDoorBell;
    MqttText m_configCodeHandsetLiftup;
    MqttText m_configCodeDoorOpener;
    MqttText m_configCodeApartmentPatternDetect;
    MqttText m_configCodeEntryPatternDetect;
    MqttText m_configCodePartyMode;

    // Diagnostics Elements
    MqttButton m_diagnosticsResetButton;
    MqttSensor m_diagnosticsRestartCounter;
    MqttSensor m_diagnosticsWifiDisconnectCounter;
    MqttSensor m_diagnosticsMqttDisconnectCounter;
    MqttSensor m_diagnosticsBssid;
    

    void publishConfig(MqttEntity &entity)
    {
        String payload = entity.getHomeAssistantConfigPayload();
        char topic[255];
        entity.getHomeAssistantConfigTopic(topic, sizeof(topic));
        if (!m_client->publish(topic, payload.c_str()))
        {
            log_error("Failed to publish config to %s", entity.getStateTopic());
        }
        entity.getHomeAssistantConfigTopicAlt(topic, sizeof(topic));
        if (!m_client->publish(topic, payload.c_str()))
        {
            log_error("Failed to publish config to %s", entity.getStateTopic());
        }
    }

    void publishMqttConfigState(MqttEntity &entity, const uint32_t value)
    {
        char state[9];
        snprintf(state, sizeof(state), "%08x", value);
        if (!m_client->publish(entity.getStateTopic(), state))
        {
            log_error("Failed to publish state to %s", entity.getStateTopic());
        }
    }

    void publishMqttCounterState(MqttEntity &entity, const uint32_t value)
    {
        char state[9];
        snprintf(state, sizeof(state), "%u", value);
        m_client->publish(entity.getStateTopic(), state);
    }

    
};