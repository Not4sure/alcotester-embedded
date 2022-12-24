//
// Created by notsure on 01.06.22.
//

#pragma once


#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "Sensor.h"

#include "Wifi.h"

class WebSocket {
public:
    enum class state_e {
        DISCONNECTED,
        WAITING_FOR_MESURE,
        CONNECTED
    };


    WebSocket(const char* uri, const int port = 80): uri{uri}, port{port} {
        config.uri = uri;
        config.port = port;
    };

    static esp_err_t send(const char* message);

    esp_err_t init();

    state_e get_state() { return state; }

private:
    Sensor sensor{};

    esp_websocket_client_config_t config;

    static esp_websocket_client_handle_t client;
    static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

    [[noreturn]] static void ws_task(void *arg);

    const char* uri;
    const int   port;

    static state_e state;
};
