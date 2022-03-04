#pragma once
#include <Arduino.h>
#include "utils.h"

enum mqtt_topic
{
    MQTT_TOPIC_NONE,

    MQTT_TOPIC_ENTRY_DOOR_BELL,
    MQTT_TOPIC_ENTRY_DOOR_BELL_PATTERN,
    MQTT_TOPIC_ENTRY_DOOR_OPENER,

    MQTT_TOPIC_APARTMENT_DOOR_BELL,
    MQTT_TOPIC_APARTMENT_DOOR_BELL_PATTERN,

    MQTT_TOPIC_PARTY_MODE, 

    MQTT_TOPIC_BUS,
};

const String mqtt_topic_s[] = {
    "",

    "entry/bell",
    "entry/bell/pattern",
    "entry/opener",

    "apartment/bell",
    "apartment/bell/pattern",

    "partymode", 

    "bus"};

enum mqtt_action
{
    MQTT_ACTION_NONE,
    MQTT_ACTION_CMD,
    MQTT_ACTION_STATE
};

const String mqtt_action_s[] = {
    "",
    "cmd",
    "state"};

enum mqtt_state
{
    MQTT_STATE_NONE,
    MQTT_STATE_ON,
    MQTT_STATE_OFF
};

const String mqtt_state_s[] = {
    "",
    "on",
    "off"};

    String mqttTopic(mqtt_topic path, mqtt_action action)
{
    String topic = "";
    topic += composeClientID();
    if (path != MQTT_TOPIC_NONE)
    {
        topic += "/";
        topic += mqtt_topic_s[path];
    }
    if (action != MQTT_ACTION_NONE)
    {
        topic += "/";
        topic += mqtt_action_s[action];
    }
    return topic;
}