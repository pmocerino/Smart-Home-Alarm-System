#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "timex.h"
#include "ztimer.h"
#include "thread.h"
#include "mutex.h"
#include "paho_mqtt.h"
#include "MQTTClient.h"

#include "thread.h"
#include "xtimer.h"
#include "periph/gpio.h"
#include "periph/adc.h"
#include "analog_util.h"

#include <inttypes.h>

#define ADC_IN_USE ADC_LINE(0) // pin 36
#define ADC_RES ADC_RES_12BIT

#define LED_PIN_NUMBER 12
#define BUZZER_PIN_NUMBER 23
#define BUTTON_PIN_NUMBER 2


// Paho-MQTT code

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#define BUF_SIZE                        1024
#define MQTT_VERSION_v311               4       /* MQTT v3.1.1 version is 4 */

#define COMMAND_TIMEOUT_MS              4000

#ifndef DEFAULT_MQTT_CLIENT_ID
#define DEFAULT_MQTT_CLIENT_ID          ""
#endif

#ifndef DEFAULT_MQTT_USER
#define DEFAULT_MQTT_USER               ""
#endif

#ifndef DEFAULT_MQTT_PWD
#define DEFAULT_MQTT_PWD                ""
#endif

/**
 * @brief Default MQTT port
 */    

#define DEFAULT_MQTT_PORT               1883
#define BROKER_IP_ADDRESS               "INSERT YOUR IP ADRRESS"

/**
 * @brief Keepalive timeout in seconds
 */

#define DEFAULT_KEEPALIVE_SEC           60

#ifndef MAX_LEN_TOPIC
#define MAX_LEN_TOPIC                   100
#endif

#ifndef MAX_TOPICS
#define MAX_TOPICS                      4
#endif

#define IS_CLEAN_SESSION                1
#define IS_RETAINED_MSG                 0

static MQTTClient client;
static Network network;
static int topic_cnt = 0;
static char _topic_to_subscribe[MAX_TOPICS][MAX_LEN_TOPIC];


static int DEBUG = 0;
static int SYSTEM = 0;
static int ALARM = 0;

static unsigned get_qos(const char *str)
{
    int qos = atoi(str);

    switch (qos) {
    case 1:     return QOS1;
    case 2:     return QOS2;
    default:    return QOS0;
    }
}


static void _on_msg_received(MessageData *data)
{
    printf("smart home alarm system: message received on topic"
           " %.*s: %.*s\n",
           (int)data->topicName->lenstring.len,
           data->topicName->lenstring.data, (int)data->message->payloadlen,
           (char *)data->message->payload);
    
    if (strcmp((char*)data->message->payload, "{\"system\": \"1\"}") == 0) SYSTEM = 1;
    else if (strcmp((char*)data->message->payload, "{\"system\": \"0\"}") == 0) SYSTEM = 0;
}

static int _cmd_discon(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    topic_cnt = 0;
    int res = MQTTDisconnect(&client);
    if (res < 0) {
        printf("smart home alarm system: Unable to disconnect\n");
    }
    else {
        printf("smart home alarm system: Disconnect successful\n");
    }

    NetworkDisconnect(&network);
    return res;
}

static int _cmd_con(int argc, char **argv)
{

    if (argc < 2) {
        printf(
            "usage: %s <brokerip addr> [port] [clientID] [user] [password] "
            "[keepalivetime]\n",
            argv[0]);
        return 1;
    }

    char *remote_ip = argv[1];

    int ret = -1;

    // ensure client isn't connected in case of a new connection 
    if (client.isconnected) {
        printf("smart home alarm system: client already connected, disconnecting it\n");
        MQTTDisconnect(&client);
        NetworkDisconnect(&network);
    }

    int port = DEFAULT_MQTT_PORT;
    if (argc > 2) {
        port = atoi(argv[2]);
    }

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VERSION_v311;

    data.clientID.cstring = DEFAULT_MQTT_CLIENT_ID;
    if (argc > 3) {
        data.clientID.cstring = argv[3];
    }

    data.username.cstring = DEFAULT_MQTT_USER;
    if (argc > 4) {
        data.username.cstring = argv[4];
    }

    data.password.cstring = DEFAULT_MQTT_PWD;
    if (argc > 5) {
        data.password.cstring = argv[5];
    }

    data.keepAliveInterval = DEFAULT_KEEPALIVE_SEC;
    if (argc > 6) {
        data.keepAliveInterval = atoi(argv[6]);
    }

    data.cleansession = IS_CLEAN_SESSION;
    data.willFlag = 0;

    printf("smart home alarm system: Connecting to MQTT Broker from %s %d\n",
            remote_ip, port);
    printf("smart home alarm system: Trying to connect to %s, port: %d\n",
            remote_ip, port);
    ret = NetworkConnect(&network, remote_ip, port);
    if (ret < 0) {
        printf("smart home alarm system: Unable to connect\n");
        return ret;
    }

    printf("user:%s clientId:%s password:%s\n", data.username.cstring,
             data.clientID.cstring, data.password.cstring);
    ret = MQTTConnect(&client, &data);
    if (ret < 0) {
        printf("smart home alarm system: Unable to connect client %d\n", ret);
        _cmd_discon(0, NULL);
        return ret;
    }
    else {
        printf("smart home alarm system: Connection successfully\n");
    }

    return (ret > 0) ? 0 : 1;
}

static int _cmd_pub(int argc, char **argv)
{
    enum QoS qos = QOS0;

    if (argc < 3) {
        printf("usage: %s <topic name> <string msg> [QoS level]\n",
               argv[0]);
        return 1;
    }
    if (argc == 4) {
        qos = get_qos(argv[3]);
    }
    MQTTMessage message;
    message.qos = qos;
    message.retained = IS_RETAINED_MSG;
    message.payload = argv[2];
    message.payloadlen = strlen(message.payload);

    int rc;
    if ((rc = MQTTPublish(&client, argv[1], &message)) < 0) {
        printf("smart home alarm system: Unable to publish (%d)\n", rc);
    }
    else {
        printf("smart home alarm system: Message (%s) has been published to topic %s "
               "with QOS %d\n",
               (char *)message.payload, argv[1], (int)message.qos);
    }

    return rc;
}

static int _cmd_sub(int argc, char **argv)
{
    enum QoS qos = QOS2;

    if (argc < 2) {
        printf("usage: %s <topic name> [QoS level]\n", argv[0]);
        return 1;
    }

    if (argc >= 3) {
        qos = get_qos(argv[2]);
    }

    if (topic_cnt > MAX_TOPICS) {
        printf("smart home alarm system: Already subscribed to max %d topics,"
                "call 'unsub' command\n", topic_cnt);
        return -1;
    }

    if (strlen(argv[1]) > MAX_LEN_TOPIC) {
        printf("smart home alarm system: Not subscribing, topic too long %s\n", argv[1]);
        return -1;
    }
    strncpy(_topic_to_subscribe[topic_cnt], argv[1], strlen(argv[1]));

    printf("smart home alarm system: Subscribing to %s\n", _topic_to_subscribe[topic_cnt]);
    int ret = MQTTSubscribe(&client,
              _topic_to_subscribe[topic_cnt], qos, _on_msg_received);
    if (ret < 0) {
        printf("smart home alarm system: Unable to subscribe to %s (%d)\n",
               _topic_to_subscribe[topic_cnt], ret);
        _cmd_discon(0, NULL);
    }
    else {
        printf("smart home alarm system: Now subscribed to %s, QOS %d\n",
               argv[1], (int) qos);
        topic_cnt++;
    }
    return ret;
}

/*
static int _cmd_unsub(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage %s <topic name>\n", argv[0]);
        return 1;
    }

    int ret = MQTTUnsubscribe(&client, argv[1]);

    if (ret < 0) {
        printf("smart home alarm system: Unable to unsubscribe from topic: %s\n", argv[1]);
        _cmd_discon(0, NULL);
    }
    else {
        printf("smart home alarm system: Unsubscribed from topic:%s\n", argv[1]);
        topic_cnt--;
    }
    return ret;
}
*/

#define DEVICE_ID                       "1"
#define MAX_DEVICE_ID_LENGTH             4
#define MAX_VALUE_LENGTH                 6

static unsigned char buf[BUF_SIZE];
static unsigned char readbuf[BUF_SIZE];

// Pins definition
gpio_t LED_PIN;
gpio_t BUZZER_PIN;
gpio_t BUTTON_PIN;

char led_stack[THREAD_STACKSIZE_MAIN];
kernel_pid_t led_thread;

char buzzer_stack[THREAD_STACKSIZE_MAIN];
kernel_pid_t buzzer_thread;

char sub_stack[THREAD_STACKSIZE_MAIN];
kernel_pid_t sub_thread;

char activation_stack[THREAD_STACKSIZE_MAIN];
kernel_pid_t activation_thread;

char system_stack[THREAD_STACKSIZE_MAIN];
kernel_pid_t system_thread;

char deactivation_stack[THREAD_STACKSIZE_MAIN];
kernel_pid_t deactivation_thread;

// Setup MQTT topics with device ID info
char MQTT_TOPIC_SYSTEM[19 + MAX_DEVICE_ID_LENGTH];
char MQTT_TOPIC_ALARM[19 + MAX_DEVICE_ID_LENGTH];


void *led_blink(void*) 
{
    while(1) {
        if (DEBUG) printf("Set LED pin to HIGH\n");

        gpio_set(LED_PIN);
        xtimer_sleep(1);

        if (DEBUG) printf("Set LED pin to LOW\n");

        gpio_clear(LED_PIN);
        xtimer_sleep(1);

        if (ALARM == 0) thread_sleep();
    }
}

void *buzzer_on(void*) 
{
    while (1) {
        if (DEBUG) printf("Set buzzer pin to HIGH\n");

        gpio_set(BUZZER_PIN);
        xtimer_sleep(1);

        if (DEBUG) printf("Set buzzer pin to LOW\n");

        gpio_clear(BUZZER_PIN);   
        xtimer_sleep(1);

        if (ALARM == 0) thread_sleep();
    }
}

void *periodical_subscribe(void*)
{
    /* Function to be used only if remote activation is needed*/
    while (true) {
        
        // Connect to MQTT broker
        char* con_arg_list[3] = {"con", BROKER_IP_ADDRESS, "1883"};
        char** con_argv = (char**)&con_arg_list;
        _cmd_con(3, con_argv);

        // Subscribe to local MQTT system topic for remote activation of the system
        char* sub_arg_list[2] = {"sub", MQTT_TOPIC_SYSTEM};
        char** sub_argv = (char**)&sub_arg_list;
        _cmd_sub(2, sub_argv);
        
        xtimer_sleep(60);
        
    }
}

void *system_activation(void*) 
{   
    printf("Press the physical or dashboard button to start the alarm system.\n");

    while (1) {
        uint32_t b_start = xtimer_now_usec();
        int button_value = gpio_read(BUTTON_PIN);
        if (DEBUG) printf("Button value is: %d\n", button_value);
        uint32_t b_end = xtimer_now_usec();
        if (DEBUG) printf("Elapsed time for button reading (in microseconds): %ld\n", b_end - b_start);


        if (button_value == 1 || SYSTEM == 1) {

            printf("System will be active in 5 seconds.\n");
            xtimer_sleep(5);

            if (SYSTEM != 1) {
                
                char* message = "{\"system\":\"1\"}";

                // Connect to MQTT broker
                // Needed if periodical connect and subscribe are not executed
                /*char* con_arg_list[3] = {"con", BROKER_IP_ADDRESS, "1883"};
                char** con_argv = (char**)&con_arg_list;
                _cmd_con(3, con_argv);*/

                // Publish to MQTT topic
                char* pub_arg_list[3] = {"pub", MQTT_TOPIC_SYSTEM, message};
                char** pub_argv = (char**)&pub_arg_list;
                _cmd_pub(3, pub_argv);


                SYSTEM = 1;

            }

            thread_wakeup(system_thread);
            thread_wakeup(deactivation_thread);
            printf("\nSystem is active.\n");
            thread_sleep();
        }

        xtimer_sleep(3);
    }
}


void *system_deactivation(void*) 
{
    printf("Press the physical or dashboard button to stop the alarm system.\n");

    while (1) {
        int button_value = gpio_read(BUTTON_PIN);
        if (DEBUG) printf("Button value is: %d\n", button_value);

        if (button_value == 1 || SYSTEM == 0) {

            ALARM = 0;

            if (SYSTEM != 0) {
                 char* message = "{\"system\":\"0\"}";

                // Connect to MQTT broker
                // Needed if periodical connect and subscribe are not executed
                /*char* con_arg_list[3] = {"con", BROKER_IP_ADDRESS, "1883"};
                char** con_argv = (char**)&con_arg_list;
                _cmd_con(3, con_argv);*/

                // Publish to MQTT topic
                char* pub_arg_list[3] = {"pub", MQTT_TOPIC_SYSTEM, message};
                char** pub_argv = (char**)&pub_arg_list;
                _cmd_pub(3, pub_argv);

                SYSTEM = 0;
            }
    
            thread_wakeup(activation_thread);
            printf("\nSystem deactivated.\n");
            thread_sleep();
        }

        xtimer_sleep(3);
    }
}

void *system_sampling(void*) 
{   
    while (1) {

        if (SYSTEM == 0) thread_sleep();

        uint32_t s_start = xtimer_now_usec();

        int sample = 0;

        // 10 measurements and mean computation
        long measure = 0;
        for (int i = 0; i < 10; i++) {
            sample = adc_sample(ADC_IN_USE, ADC_RES);
            measure += sample;
        }
        // Voltage mean in mV
        long out_v = measure / 10; 
        printf("Mean output voltage: %ld mv\n", out_v);

        // The output voltage is lower than 2400 mV (lower if using more magnets or being closer to the sensor) for closed door (magnets close to sensor)
        // When the door is opened, the value of the output voltage is around 3000 mV
        // A good threshold value is 2700 mV (about 140 000 mT), in order to avoid false negatives

        // Flux density
        long m_flux = out_v * 53.33 - 133.3;
        printf("Magnetic flux density: %ld mT\n", m_flux);

        uint32_t s_end = xtimer_now_usec();
        if (DEBUG) printf("Elapsed sampling time (in microseconds): %ld\n", s_end - s_start);


        long threshold = 140000; // Magnetic flux for output voltage of 2700 mV
        if (m_flux > threshold || ALARM) {
            printf("\n\nALARM: INTRUSION DETECTED!\n\n");
            if (!ALARM) {
                ALARM = 1;
                printf("Press the physical or dashboard button to stop the alarm system.\n");
                thread_wakeup(led_thread);
                thread_wakeup(buzzer_thread);
            }

            // String conversion
            char* s_value = (char*)malloc(MAX_VALUE_LENGTH * sizeof(char));
            sprintf(s_value, "%" PRIu32 "", m_flux);

            int m_len = 28 + MAX_VALUE_LENGTH;
            char* message = malloc(m_len * sizeof(char));

            sprintf(message, "{\"alarm\":\"1\", \"m_field\":\"%s\"}", s_value);

            uint32_t pub_start = xtimer_now_usec();

            // Connect to MQTT broker
            // Needed if periodical connect and subscribe are not executed
            /*char* con_arg_list[3] = {"con", BROKER_IP_ADDRESS, "1883"};
            char** con_argv = (char**)&con_arg_list;
            _cmd_con(3, con_argv);*/

            // Publish 
            char* pub_arg_list[3] = {"pub", MQTT_TOPIC_ALARM, message};
            char** pub_argv = (char**)&pub_arg_list;
            _cmd_pub(3, pub_argv);

            uint32_t pub_end = xtimer_now_usec();
            printf("Elapsed publishing time (in microseconds): %ld\n", pub_end - pub_start);

            free(s_value);
            free(message);
            
        }

        xtimer_sleep(2);
    }
}


int main(void) {

    printf("\n ---- SMART HOME ALARM SYSTEM ---- \n");

    // Initialize gpio port out 
    
    // LED pin
    LED_PIN = GPIO_PIN(PORT_GPIO, LED_PIN_NUMBER);

    if (gpio_init(LED_PIN, GPIO_OUT) && DEBUG) {
        printf("Error to initialize GPIO_PIN(%d %d)\n", PORT_GPIO, LED_PIN_NUMBER);
        return -1;
    }

    // Active buzzer pin
    BUZZER_PIN = GPIO_PIN(PORT_GPIO, BUZZER_PIN_NUMBER);

    if (gpio_init(BUZZER_PIN, GPIO_OUT) && DEBUG) {
        printf("Error to initialize GPIO_PIN(%d %d)\n", PORT_GPIO, BUZZER_PIN_NUMBER);
        return -1;
    }

    // Button pin
    BUTTON_PIN = GPIO_PIN(PORT_GPIO, BUTTON_PIN_NUMBER);
    if (gpio_init(BUTTON_PIN, GPIO_IN_PU) && DEBUG) {
        printf("Error to initialize GPIO_PIN(%d %d)\n", PORT_GPIO, BUTTON_PIN_NUMBER);
        return -1;
    }

    // Initialize the ADC line
    if (adc_init(ADC_IN_USE) < 0 && DEBUG) {
        printf("Failed to initialize ADC");
        return 1;
    }

    // MQTT topic names initialization
    snprintf(MQTT_TOPIC_SYSTEM, sizeof(MQTT_TOPIC_SYSTEM), "home/doors/%s/system", DEVICE_ID);
    snprintf(MQTT_TOPIC_ALARM, sizeof(MQTT_TOPIC_ALARM), "home/doors/%s/alarm", DEVICE_ID);

    // Paho MQTT
    if (IS_USED(MODULE_GNRC_ICMPV6_ECHO)) {
        msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    }

    #ifdef MODULE_LWIP
        // Let LWIP initialize
        ztimer_sleep(ZTIMER_MSEC, 1 * MS_PER_SEC);
    #endif

    NetworkInit(&network);

    MQTTClientInit(&client, &network, COMMAND_TIMEOUT_MS, buf, BUF_SIZE,
                   readbuf,
                   BUF_SIZE);
    printf("Running smart home alarm system.\n");

    MQTTStartTask(&client);

    xtimer_sleep(3);

    // Creation of needed threads
    led_thread = thread_create(led_stack, sizeof(led_stack), THREAD_PRIORITY_MAIN,
                                THREAD_CREATE_SLEEPING, led_blink, NULL, "led_activation");
    buzzer_thread = thread_create(buzzer_stack, sizeof(buzzer_stack), THREAD_PRIORITY_MAIN, 
                                THREAD_CREATE_SLEEPING, buzzer_on, NULL, "buzzer_activation");

    sub_thread = thread_create(sub_stack, sizeof(sub_stack), THREAD_PRIORITY_MAIN,
                                THREAD_CREATE_SLEEPING, periodical_subscribe, NULL, "periodical subscribe");
    activation_thread = thread_create(activation_stack, sizeof(activation_stack), THREAD_PRIORITY_MAIN, 
                                THREAD_CREATE_SLEEPING, system_activation, NULL, "system_start");
    system_thread = thread_create(system_stack, sizeof(system_stack), THREAD_PRIORITY_MAIN, 
                                THREAD_CREATE_SLEEPING, system_sampling, NULL, "system");
    deactivation_thread = thread_create(deactivation_stack, sizeof(deactivation_stack), THREAD_PRIORITY_MAIN, 
                                THREAD_CREATE_SLEEPING, system_deactivation, NULL, "system_end");

    xtimer_sleep(3);
    
    thread_wakeup(sub_thread);

    // Start system, all other threads are cross-referenced
    thread_wakeup(activation_thread);

    // This should never be reached
    return 0;

}
