# Smart Alarm System

| Author | Linkedin | Github |
|--- |--- |--- |
| Pasquale Mocerino | [Linkedin](https://www.linkedin.com/in/pasquale-mocerino-29088b1a2/) | [Github](https://github.com/pasqualemocerino) |

## Description

Cloud-based home alarm IoT system collecting information from a set of Hall effect sensor placed on fixtures and interacting with the environment using actuators following the 'Sense-Think-Act' paradigm.

My project is designed in order to offer some smart functionalities to a portable and flexible alarm system for buildings to keep track of internal break-ins or intrusions, for rapid notification. In detail, the system measures the opening of fixtures and provides a proper response through the activation of a LED and a buzzer and the sending of a notification to the user.

IoT devices are developed using RIOT-OS, while for cloud services I used the AWS ecosystem. A description of the system architecture is provided, as well as a performance evalution, with some considerations about possible optimizations.

I focused on a single node, in order to work on a real prototype of the system.

## Useful links

[Concept](https://github.com/pasqualemocerino/Smart-Home-Alarm-System/blob/main/Concept.md), document providing details on the system from the user point of view.\
[Design](https://github.com/pasqualemocerino/Smart-Home-Alarm-System/blob/main/Design.md), document providing details on techincal aspects of the system.\
[Evaluation](https://github.com/pasqualemocerino/Smart-Home-Alarm-System/blob/main/Evaluation.md), document providing details on system performance.\
[Hackster.io blog post](https://www.hackster.io/mocerino1919964/smart-home-alarm-system-e8c79e), containing an hands-on step-by-step explaination of the project and a tutorial of the steps to follow.\
[YouTube video demonstration](), a short video showing how the system works.

## Startup Guide
Install RIOT-OS from the official GitHub source with:

    git clone https://github.com/RIOT-OS/RIOT

In the same directory, clone my repository:

    git clone https://github.com/pasqualemocerino/Smart-Home-Alarm-System

After configuring your local Mosquitto MQTT Broker, set the local IP address in given code. After retrieving it, for example by running:

    hostname -I

Insert it into the code/MQTT_Bridge/conf/broker_ip.conf and into the RIOT-OS code/main.c file, by inserting it in the line:

    #define BROKER_IP_ADDRESS               "INSERT YOUR IP ADDRESS"

Furthermore, set your Wi-Fi SSID and password in code/Makefile:

    WIFI_SSID ?= "INSERT YOUR WIFI SSID"  
    WIFI_PASS ?= "INSERT YOUR WIFI PASSWORD"

Finally, it is needed to edit the code/MQTT_Bridge/mqtt_bridge.py file, in detail the following lines:
    
    AWS_IOT_ENDPOINT = "INSERT YOUR AWS IOT ENDPOINT"
    
    AWS_IOT_ROOT_CA = "ABSOLUTE PATH TO root-CA.crt"
    AWS_IOT_PRIVATE_KEY = "ABSOLUTE PATH TO esp32.private.key"
    AWS_IOT_CERTIFICATE = "ABSOLUTE PATH TO alarm_esp32.cert.pem"

To test the system, from a first terminal, flash the firmware to the MCU and start it:

    cd Smart-Home-Alarm-System/code/
    BOARD="esp32-heltec-lora32-v2"  BUILD_IN_DOCKER=1 DOCKER="sudo docker" make all flash term

From another terminal, start the Python MQTT Bridge:

    cd Smart-Home-Alarm-System/code/MQTT_Bridge/
    python3 mqtt_bridge.py

A step-by-step implementation explaination and tutorial is provided in the previously linked Hackster.io blog post.
