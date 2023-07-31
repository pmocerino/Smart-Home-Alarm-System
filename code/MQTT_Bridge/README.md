# MQTT Bridge
Directory containing Python code for MQTT Bridge, useful to redirect sensor values to AWS.

## Instructions
After downloading [mosquitto](https://github.com/eclipse/mosquitto), it is possible to connect to AWS following the official [guide](https://aws.amazon.com/it/blogs/iot/how-to-bridge-mosquitto-mqtt-broker-to-aws-iot/).

The MQTT bridge must be run locally in a terminal separated from the one executing RIOT code. The commands to run are the following:

    cd /Smart-Home-Alarm-System/code/MQTT_Bridge/
    python3 mqtt_bridge.py
