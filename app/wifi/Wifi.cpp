//
// Created by notsure on 10.05.22.
//

#include "Wifi.h"

namespace WIFI {

    char                Wifi::mac_addr_cstr[]{};
    std::mutex          Wifi::init_mutex{};
    std::mutex          Wifi::connect_mutex{};
    std::mutex          Wifi::state_mutex{};
    Wifi::state_e       Wifi::_state{state_e::NOT_INITIALISED};
    wifi_init_config_t  Wifi::wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t       Wifi::wifi_config{};

    NVS::Nvs            Wifi::storage{};

    Wifi::Wifi() {
        ESP_LOGI(_log_tag, "%s: Waiting for init mutex", __func__);
        std::lock_guard<std::mutex> guard(init_mutex);

        if(!get_mac()[0])
            if(ESP_OK != _get_mac())
                esp_restart();
    }

    void Wifi::wifi_event_handler(void *ard, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if(WIFI_EVENT == event_base) {
            ESP_LOGI(_log_tag, "%s: Got wifi event; ID: %d", __func__, event_id);

            const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)};

            switch (event_type) {
                case WIFI_EVENT_STA_START: {
                    ESP_LOGI(_log_tag, "%s: STA_START, waiting for state_mutex", __func__);
                    std::lock_guard<std::mutex> guard(state_mutex);
                    _state = state_e::READY_TO_CONNECT;

                    ESP_LOGI(_log_tag, "%s: READY_TO_CONNECT", __func__);
                    break;
                }
                case WIFI_EVENT_STA_CONNECTED:{
                    ESP_LOGI(_log_tag, "%s: STA_CONNECTED, waiting for state_mutex", __func__);
                    std::lock_guard<std::mutex> guard1(state_mutex);
                    _state = state_e::WAITING_FOR_IP;

                    ESP_LOGI(_log_tag, "%s: WAITING_FOR_IP", __func__);
                    break;
                }
                default:
                    ESP_LOGW(_log_tag, "%s: Default switch case (%d)", __func__, event_id);
                    break;
            }
        }
    }

    void Wifi::ip_event_handler(void *ard, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if(IP_EVENT == event_base) {
            ESP_LOGI(_log_tag, "%s: Got ip event; ID: %d", __func__, event_id);

            const ip_event_t event_type{static_cast<ip_event_t>(event_id)};

            switch (event_type) {
                case IP_EVENT_STA_GOT_IP: {
                    ESP_LOGI(_log_tag, "%s: GOT_IP, waiting for state_mutex", __func__);

                    std::lock_guard<std::mutex> guard(state_mutex);
                    _state = state_e::CONNECTED;

                    ESP_LOGI(_log_tag, "%s: CONNECTED", __func__);
                    break;
                }
                case IP_EVENT_STA_LOST_IP: {
                    ESP_LOGI(_log_tag, "%s: LOST_IP, waiting for state_mutex", __func__);

                    std::lock_guard<std::mutex> guard1(state_mutex);
                    _state = state_e::WAITING_FOR_IP;

                    ESP_LOGI(_log_tag, "%s: WAITING_FOR_IP", __func__);
                    break;
                }
                default:
                    ESP_LOGW(_log_tag, "%s: Default in switch/case", __func__);
                    break;
            }
        }
    }

    esp_err_t Wifi::_init() {
        ESP_LOGI(_log_tag, "%s: Waiting for init_mutex", __func__);
        std::lock_guard<std::mutex> init_guard(init_mutex);

        ESP_LOGI(_log_tag, "%s: Waiting for state_mutex", __func__);
        std::lock_guard<std::mutex> state_guard(state_mutex);

        esp_err_t status{ESP_OK};
        if(state_e::NOT_INITIALISED == _state) {
            ESP_LOGI(_log_tag, "%s: Calling esp_netif_init", __func__);
            status |= esp_netif_init();
            ESP_LOGI(_log_tag, "%s: esp_netif_init: %s", __func__, esp_err_to_name(status));

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s: Calling esp_netif_create_default_wifi_sta", __func__);
                esp_netif_t* p_netif = esp_netif_create_default_wifi_sta();
                ESP_LOGI(_log_tag, "%s: esp_netif_create_default_wifi_sta: %p", __func__, p_netif);

                if(!p_netif) status = ESP_FAIL;
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s: Calling esp_wifi_init", __func__);
                status |= esp_wifi_init(&wifi_init_config);
                ESP_LOGI(_log_tag, "%s: esp_wifi_init: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s:%d Calling esp_event_handler_instance_register", __func__, __LINE__);

                status |= esp_event_handler_instance_register(
                        WIFI_EVENT,
                        ESP_EVENT_ANY_ID,
                        &wifi_event_handler,
                        nullptr, nullptr);
                ESP_LOGI(_log_tag, "%s: esp_event_handler_instance_register: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s:%d Calling esp_event_handler_instance_register", __func__, __LINE__);

                status |= esp_event_handler_instance_register(
                        IP_EVENT,
                        ESP_EVENT_ANY_ID,
                        &ip_event_handler,
                        nullptr, nullptr);
                ESP_LOGI(_log_tag, "%s: esp_event_handler_instance_register: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s: Calling esp_wifi_set_mode", __func__);

                status |= esp_wifi_set_mode(WIFI_MODE_STA);
                ESP_LOGI(_log_tag, "%s: esp_wifi_set_mode: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {

                memcpy(wifi_config.sta.ssid, ssid, std::min(strlen(ssid), sizeof(wifi_config.sta.ssid)));
                memcpy(wifi_config.sta.password, password, std::min(strlen(password), sizeof(wifi_config.sta.password)));
                wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
                wifi_config.sta.pmf_cfg.capable = true;
                wifi_config.sta.pmf_cfg.required = false;

                ESP_LOGI(_log_tag, "%s: Calling esp_wifi_set_config", __func__);
                status |= esp_wifi_set_config(WIFI_IF_STA,  &wifi_config);
                ESP_LOGI(_log_tag, "%s: esp_wifi_set_config: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s: Calling esp_wifi_start", __func__);
                status |= esp_wifi_start();
                ESP_LOGI(_log_tag, "%s: esp_wifi_start: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s: INITIALISED", __func__);
                _state = state_e::INITIALISED;
            }
        } else if (state_e::ERROR == _state) {
            ESP_LOGE(_log_tag, "%s: FAILED", __func__);
            status = ESP_FAIL;
        }

        return status;
    }

    esp_err_t Wifi::begin() {
        ESP_LOGI(_log_tag, "%s: Waiting for connect_mutex", __func__);
        std::lock_guard<std::mutex> connect_guard(connect_mutex);

        esp_err_t status{ESP_OK};

        ESP_LOGI(_log_tag, "%s: Waiting for state_mutex", __func__);
        std::lock_guard<std::mutex> state_guard(state_mutex);
        switch (_state) {
            case state_e::READY_TO_CONNECT:
                ESP_LOGI(_log_tag, "%s: Calling esp_wifi_connect", __func__);
                status = esp_wifi_connect();
                ESP_LOGI(_log_tag, "%s: esp_wifi_connect: %s", __func__, esp_err_to_name(status));

                if(ESP_OK == status)
                    _state = state_e::CONNECTING;
            case state_e::CONNECTING:
            case state_e::WAITING_FOR_IP:
            case state_e::CONNECTED:
                break;
            case state_e::NOT_INITIALISED:
            case state_e::INITIALISED:
            case state_e::WAITING_FOR_CREDENTIALS:
            case state_e::DISCONNECTING:
            case state_e::ERROR:
                ESP_LOGE(_log_tag, "%s: Error state", __func__);
                status = ESP_FAIL;
                break;
        }

        if(state_e::READY_TO_CONNECT == _state) {
            status = esp_wifi_connect();
        }

        return status;
    }

    esp_err_t Wifi::_get_mac() {
        uint8_t buffer[6]{};

        const esp_err_t status{esp_efuse_mac_get_default(buffer)};
        if (ESP_OK == status)
            snprintf(mac_addr_cstr, sizeof(mac_addr_cstr), "%02X%02X%02X%02X%02X%02X",
                    buffer[0], buffer[1], buffer[2],
                    buffer[3], buffer[4], buffer[5]);
        return status;
    }
}