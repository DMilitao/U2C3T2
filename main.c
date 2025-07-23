#include "main.h"

static MQTT_CLIENT_T* mqtt_client_init(void) {
    MQTT_CLIENT_T *state = calloc(1, sizeof(MQTT_CLIENT_T));
    if (!state) {
        DEBUG_printf("Failed to allocate state\n");
        return NULL;
    }
    return state;
}

void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T*)callback_arg;
    if (ipaddr) {
        state->remote_addr = *ipaddr;
        DEBUG_printf("DNS resolved: %s\n", ip4addr_ntoa(ipaddr));
    } else {
        DEBUG_printf("DNS resolution failed.\n");
    }
}

void run_dns_lookup(MQTT_CLIENT_T *state) {
    DEBUG_printf("Running DNS lookup for %s...\n", MQTT_SERVER_HOST);
    if (dns_gethostbyname(MQTT_SERVER_HOST, &(state->remote_addr), dns_found, state) == ERR_INPROGRESS) {
        while (state->remote_addr.addr == 0) {
            cyw43_arch_poll();
            sleep_ms(10);
        }
    }
}

void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        
        DEBUG_printf("MQTT connected.\n");
    } else {
        
        DEBUG_printf("MQTT connection failed: %d\n", status);
    }
}

void mqtt_pub_request_cb(void *arg, err_t err) {
    //DEBUG_printf("Publish request status: %d\n", err);
}

void mqtt_sub_request_cb(void *arg, err_t err) {
    DEBUG_printf("Subscription request status: %d\n", err);
}

err_t mqtt_test_connect(MQTT_CLIENT_T *state) {
    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = "PicoW";
    return mqtt_client_connect(state->mqtt_client, &(state->remote_addr), MQTT_SERVER_PORT, mqtt_connection_cb, state, &ci);
}

bool mqtt_connect(MQTT_CLIENT_T *state) {

    state->mqtt_client = mqtt_client_new();
    if (!state->mqtt_client) {
        DEBUG_printf("Failed to create MQTT client\n");
        return false;
    }

    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = "PicoW";

    err_t err = mqtt_client_connect(state->mqtt_client, &(state->remote_addr), MQTT_SERVER_PORT, mqtt_connection_cb, state, &ci);

    if ( err ) {
        DEBUG_printf("Failed to connect to server");
        return false;
    }
    
    int i = 0;
    while ( true ) {
        DEBUG_printf("Testing connection....\n");
        cyw43_arch_poll();
        sleep_ms(500);
        i++;
        if ( mqtt_client_is_connected(state->mqtt_client) ){
            break;
        }
        if ( i > 100 ) {
            return false;
        }
        
    }
    
    mqtt_publish_topic(state,"pico_w/test_init","I am connected !");
    mqtt_set_inpub_callback(state->mqtt_client, mqtt_pub_start_cb, mqtt_pub_data_cb, NULL);
    mqtt_sub_unsub(state->mqtt_client, "pico_w/receive", 0, mqtt_sub_request_cb, NULL, 1);

    cyw43_arch_poll();

    return true;
}

err_t mqtt_publish_topic(MQTT_CLIENT_T *state, char *topic, char *message) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now - last_time >= PUB_DELAY_MS) {

        if (!mqtt_client_is_connected(state->mqtt_client)) {
            printf("Client is not connected!\n");
            return ERR_CONN;
        }

        //DEBUG_printf("%d - Publicando no tópico [%s]: %s\n", now-last_time, topic, message);
        last_time = now;
        return mqtt_publish( state->mqtt_client, topic, message, strlen(message), 0, 0, mqtt_pub_request_cb, state );
    }

    return ERR_OK;
}

void mqtt_pub_start_cb(void *arg, const char *topic, u32_t tot_len) {
    DEBUG_printf("Incoming message on topic: %s\n", topic);
}

void mqtt_pub_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    char buffer[BUFFER_SIZE];
    if (len < BUFFER_SIZE) {
        memcpy(buffer, data, len);
        buffer[len] = '\0';
        DEBUG_printf("Message received: %s\n", buffer);
        if (strcmp(buffer, "on") == 0) {
            gpio_put(LED, 1);
        } else if (strcmp(buffer, "off") == 0) {
            gpio_put(LED, 0);
        }
    } else {
        DEBUG_printf("Message too large, discarding.\n");
    }
}


int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(JS_x);

    gpio_init(BTN);
    gpio_set_dir(BTN, GPIO_IN);
    gpio_pull_up(BTN);

    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_put(LED, 0);

    if (cyw43_arch_init()) {
        DEBUG_printf("Failed to initialize WiFi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        DEBUG_printf("Failed to connect to WiFi\n");
    }

    DEBUG_printf("WiFi connected \n");

    MQTT_CLIENT_T *state = mqtt_client_init();
    run_dns_lookup(state);

    while( !mqtt_connect(state) ){
    }

    while (true)
    {
        bool btn_state = gpio_get(BTN);
        adc_select_input(CHAN_JS_x);
        uint32_t adc_x_raw = adc_read();
        char msg[64];
        snprintf(msg, sizeof(msg), "Dig: %s | Analog: %d", (btn_state ? "true" : "false"), adc_x_raw);
        mqtt_publish_topic(state,"pico_w/mqtt",msg);
        cyw43_arch_poll();
        sleep_ms(10);
    }
    
    cyw43_arch_deinit();
    return 0;
}