import paho.mqtt.client as mqtt
import datetime
import json

def time_now():
    return datetime.datetime.now().strftime('%H:%M:%S.%f')

# MQTT client to connect to the bus
mqtt_client = mqtt.Client()


def on_connect(client, userdata, flags, rc):
    # subscribe to all messages
    client.subscribe('#')



# Process a message as it arrives
def on_message(client, userdata, msg):
    excludes = [
    "hermes/audioServer/livingroom/audioFrame",
    "hermes/audioServer/bedroom/audioFrame", 
    "hermes/audioServer/kitchen/audioFrame"
    ]
    if msg.topic in excludes:
        return
    if len(msg.payload) > 0 and len(msg.payload) < 2000 and not msg.topic in excludes:
        jsonData = json.loads(msg.payload)
        pretty = json.dumps(jsonData, indent=4, sort_keys=True)
        print('[{}] - {}: {}'.format(time_now(), msg.topic, pretty))
    else:
        print('[{}] - {}'.format(time_now(), msg.topic))

mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect('localhost', 1883)
mqtt_client.loop_forever()
