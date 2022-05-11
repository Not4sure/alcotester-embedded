//
// Created by notsure on 10.05.22.
//
#pragma once

#include <esp_err.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include <mutex>
#include <algorithm>
#include <cstring>

namespace WIFI {
    class Wifi {
        constexpr static char _log_tag[5] =  {"WiFi"};

        constexpr static char ssid[31] = {"example"};
        constexpr static char password[65] = {"example"};
    public:
        enum class state_e {
            NOT_INITIALISED,
            INITIALISED,
            WAITING_FOR_CREDENTIALS,
            READY_TO_CONNECT,
            CONNECTING,
            WAITING_FOR_IP,
            CONNECTED,
            DISCONNECTING,
            ERROR
        };

        Wifi();

        esp_err_t init();
        esp_err_t begin();

        constexpr static const state_e& get_state() { return _state; };
        constexpr static const char* get_mac() { return mac_addr_cstr; };
    private:
        static char mac_addr_cstr[13];
        static std::mutex init_mutex;
        static std::mutex connect_mutex;
        static std::mutex state_mutex;
        static state_e _state;
        static wifi_init_config_t wifi_init_config;
        static wifi_config_t wifi_config;

        void state_machine();

        static void wifi_event_handler(void* ard, esp_event_base_t event_base, int32_t event_id, void* event_data);
        static void ip_event_handler(void* ard, esp_event_base_t event_base, int32_t event_id, void* event_data);

        static esp_err_t _get_mac();
        static esp_err_t _init();
    };

};
