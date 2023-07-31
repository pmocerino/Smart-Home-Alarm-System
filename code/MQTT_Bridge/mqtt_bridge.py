# AWS SDK
from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
# Paho MQTT
import paho.mqtt.client as mqtt

import json
import signal


# MQTT broker
MQTT_BROKER_ADDR = "" # To be set in main function
MQTT_BROKER_PORT = 1883
MQTT_BROKER_CLIENT_ID = "bridgeawsiot"

# AWS constants
AWS_IOT_ENDPOINT = "INSERT YOUR AWS IOT ENDPOINT"
AWS_IOT_PORT = 8883
AWS_IOT_CLIENT_ID = "basicPubSub"
AWS_IOT_ROOT_CA = "ABSOLUTE PATH TO root-CA.crt"
AWS_IOT_PRIVATE_KEY = "ABSOLUTE PATH TO esp32.private.key"
AWS_IOT_CERTIFICATE = "ABSOLUTE PATH TO alarm_esp32.cert.pem"

# MQTT_TOPIC_DOOR = "localgateway_to_awsiot"
MQTT_TOPIC_SYSTEM = "home/doors/1/system"
MQTT_TOPIC_ALARM = "home/doors/1/alarm"

# Default topic of MQTT bridge is used, but it can be changed by properly configuring bridge.conf file
MQTT_TOPIC_SYSTEM_REMOTE = "both_directions"

# MQTT callback function
def on_message(_client, _userdata, message):

    payload = json.loads(message.payload.decode())
    print("Topic: ", message.topic, " Payload: ", payload)

    if (message.topic == MQTT_TOPIC_SYSTEM) or (message.topic == MQTT_TOPIC_ALARM):
        # Send payload to AWS
        if (message.topic == MQTT_TOPIC_SYSTEM):
            aws_payload = json.dumps({
                "system": payload['system']
            })
        else:
            aws_payload = json.dumps({
                "alarm": payload['alarm'],
                "m_field": payload['m_field']
            })

        is_pub = myMQTTClient.publish(message.topic, aws_payload, 0)

        if(is_pub):
            print("published", aws_payload)


    if (message.topic == MQTT_TOPIC_SYSTEM_REMOTE):
        # Forward payload received from remote dashboard to local MQTT broker system topic
        # In this way the system can be effectively activated
        mqtt_client.publish(MQTT_TOPIC_SYSTEM, message.payload)

# Subscription to topic on connect
def on_connect(_client, _userdata, _flags, result):

    # Subscribe to topics 
    mqtt_client.subscribe(MQTT_TOPIC_SYSTEM)
    print('Subscribed to ', MQTT_TOPIC_SYSTEM)
    mqtt_client.subscribe(MQTT_TOPIC_ALARM)
    print('Subscribed to ', MQTT_TOPIC_ALARM)
    mqtt_client.subscribe(MQTT_TOPIC_SYSTEM_REMOTE)
    print('Subscribed to ', MQTT_TOPIC_SYSTEM_REMOTE)


# Disconnect function
def disconnect(signum, frame):
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
    myMQTTClient.disconnect()
    print("Disconnected")
    exit(0)

# Register signal handler for CTRL+C
signal.signal(signal.SIGINT, disconnect)


if __name__ == '__main__':

    # Setting the broker IP address
    f = open("./conf/broker_ip.conf", "r")
    MQTT_BROKER_ADDR = f.read().strip()
    f.close()

    # For certificate based connection
    myMQTTClient = AWSIoTMQTTClient(AWS_IOT_CLIENT_ID)
    # For TLS mutual authentication
    myMQTTClient.configureEndpoint(AWS_IOT_ENDPOINT, 8883)
    myMQTTClient.configureCredentials(AWS_IOT_ROOT_CA, AWS_IOT_PRIVATE_KEY, AWS_IOT_CERTIFICATE)
    myMQTTClient.configureOfflinePublishQueueing(-1)  # Infinite offline Publish queueing
    myMQTTClient.configureDrainingFrequency(2)  # Draining: 2 Hz
    myMQTTClient.configureConnectDisconnectTimeout(10)  # 10 sec
    myMQTTClient.configureMQTTOperationTimeout(5)  # 5 sec

    # Paho MQTT client
    mqtt_client = mqtt.Client(client_id=MQTT_BROKER_CLIENT_ID)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    myMQTTClient.connect()
    mqtt_client.connect(MQTT_BROKER_ADDR, MQTT_BROKER_PORT)
    mqtt_client.loop_forever()
