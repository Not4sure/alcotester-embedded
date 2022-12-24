//
// Created by notsure on 10.05.22.
//
#pragma once

#include <cstring>
#include <algorithm>
#include <mutex>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_smartconfig.h"

#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

#include "Nvs32.h"
#include "RGBLed.h"

namespace WIFI {
    class Wifi {
        constexpr static char _log_tag[5] =  {"WiFi"};

        constexpr static char ssid[31] = {""};
        constexpr static char password[65] = {""};
    public:
        enum class state_e {
            NOT_INITIALISED,
            INITIALISED,
            STARTED,
            WAITING_FOR_CREDENTIALS,
            READY_TO_CONNECT,
            CONNECTING,
            WAITING_FOR_IP,
            CONNECTED,
            DISCONNECTED,
            ERROR
        };

        Wifi();

        static esp_err_t init() { return _init(); };
        esp_err_t begin();

        constexpr static const state_e& get_state() { return _state; };
        constexpr static const char* get_mac() { return mac_addr_cstr; };
    private:
        static state_e _state;
        static char mac_addr_cstr[13];
        static std::mutex init_mutex;
        static std::mutex connect_mutex;
        static std::mutex state_mutex;

//        static RGB::Led led;

        static wifi_init_config_t   wifi_init_config;
        static wifi_config_t        wifi_config;

        static bool empty_credentials() { return '\0' == wifi_config.sta.ssid[0]; }

        static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
        static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
        static void prov_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

        static void wifi_task(void *arg);
        static esp_err_t _get_mac();
        static esp_err_t _init();

        static NVS::Nvs storage;
    };

};
