#include <time.h>
#include <stdio.h>
#include <string.h>
#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "lwip/apps/mqtt.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt_priv.h"
#include "hardware/structs/rosc.h"

/* MACROS PI PICO */
#define BTN 5
#define JS_x 27
#define CHAN_JS_x 1
#define LED 12
/* END */

/* MACROS MQTT */
#define DEBUG_printf printf
#define MQTT_SERVER_HOST "test.mosquitto.org"
#define MQTT_SERVER_PORT 1883
#define MQTT_TLS 0
#define BUFFER_SIZE 256
#define PUB_DELAY_MS 1000
/* END*/

/*VARIAVEIS*/
typedef struct MQTT_CLIENT_T_ {
    ip_addr_t remote_addr;
    mqtt_client_t *mqtt_client;
    u32_t received;
    u32_t counter;
    u32_t reconnect;
} MQTT_CLIENT_T;
static uint32_t last_time = 0;
/* END */

/* FUNÇOES */

static MQTT_CLIENT_T* mqtt_client_init(void);
void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
void run_dns_lookup(MQTT_CLIENT_T *state);

bool mqtt_connect(MQTT_CLIENT_T *state);
err_t mqtt_publish_topic(MQTT_CLIENT_T *state, char *topic, char *message);

void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
void mqtt_pub_request_cb(void *arg, err_t err);
void mqtt_sub_request_cb(void *arg, err_t err);
void mqtt_pub_start_cb(void *arg, const char *topic, u32_t tot_len);
void mqtt_pub_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);

/* END */