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

//    RGB::Led            led{};

    Wifi::Wifi() {
        ESP_LOGI(_log_tag, "%s: Waiting for init mutex", __func__);
        std::lock_guard<std::mutex> guard(init_mutex);

        if(!get_mac()[0])
            if(ESP_OK != _get_mac())
                esp_restart();
    }

    void Wifi::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if(WIFI_EVENT == event_base) {
            ESP_LOGI(_log_tag, "%s: Got wifi event; ID: %d", __func__, event_id);

            const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)};

            switch (event_type) {
                case WIFI_EVENT_STA_START: {
                    ESP_LOGI(_log_tag, "%s: STA_START, waiting for state_mutex", __func__);
                    std::lock_guard<std::mutex> guard(state_mutex);
                    _state = state_e::STARTED;

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

    void Wifi::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
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

    void Wifi::prov_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if (WIFI_PROV_EVENT == event_base) {
            switch (event_id) {
                case WIFI_PROV_START:
                    ESP_LOGI(_log_tag, "Provisioning started");
                    break;
                case WIFI_PROV_CRED_RECV: {
                    auto *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                    ESP_LOGI(_log_tag, "Received Wi-Fi credentials"
                                       "\n\tSSID     : %s\n\tPassword : %s",
                             (const char *) wifi_sta_cfg->ssid,
                             (const char *) wifi_sta_cfg->password);
                    break;
                }
                case WIFI_PROV_CRED_FAIL: {
                    auto *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                    ESP_LOGE(_log_tag, "Provisioning failed!\n\tReason : %s"
                                       "\n\tPlease reset to factory and retry provisioning",
                             (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                             "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                    std::lock_guard<std::mutex> state_guard(state_mutex);
                    _state = state_e::ERROR;
                    break;
                }
                case WIFI_PROV_CRED_SUCCESS:{
                    ESP_LOGI(_log_tag, "Provisioning successful");
                    std::lock_guard<std::mutex> state_guard(state_mutex);
                    _state = state_e::READY_TO_CONNECT;
                    break;
                }
//                case WIFI_PROV_END:
//                    /* De-initialize manager once provisioning is finished */
//                    wifi_prov_mgr_deinit();
//                    break;
                default:
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
                ESP_LOGI(_log_tag, "%s:%d Registering wifi_event_handler", __func__, __LINE__);

                status |= esp_event_handler_instance_register(
                        WIFI_EVENT,
                        ESP_EVENT_ANY_ID,
                        &wifi_event_handler,
                        nullptr, nullptr);
                ESP_LOGI(_log_tag, "%s: Registered wifi_event_handler: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s:%d Registering ip_event_handler", __func__, __LINE__);

                status |= esp_event_handler_instance_register(
                        IP_EVENT,
                        ESP_EVENT_ANY_ID,
                        &ip_event_handler,
                        nullptr, nullptr);
                ESP_LOGI(_log_tag, "%s: Registered ip_event_handler: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s:%d Registering prov_event_handler", __func__, __LINE__);

                status |= esp_event_handler_instance_register(
                        WIFI_PROV_EVENT,
                        ESP_EVENT_ANY_ID,
                        &prov_event_handler,
                        nullptr, nullptr);
                ESP_LOGI(_log_tag, "%s: Registered prov_event_handler: %s", __func__, esp_err_to_name(status));
            }

            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s: Calling esp_wifi_set_mode", __func__);

                status |= esp_wifi_set_mode(WIFI_MODE_STA);
                ESP_LOGI(_log_tag, "%s: esp_wifi_set_mode: %s", __func__, esp_err_to_name(status));
            }


            if(ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s: Calling esp_wifi_start", __func__);
                status |= esp_wifi_start();
                ESP_LOGI(_log_tag, "%s: esp_wifi_start: %s", __func__, esp_err_to_name(status));
            }

            if (ESP_OK == status) {
                ESP_LOGI(_log_tag, "%s: Calling wifi_prov_mgr_init", __func__);

                wifi_prov_mgr_config_t config{wifi_prov_scheme_ble,
                                              WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
                                              WIFI_PROV_EVENT_HANDLER_NONE};
                status |= wifi_prov_mgr_init(config);
                ESP_LOGI(_log_tag, "%s: wifi_prov_mgr_init: %s", __func__, esp_err_to_name(status));
            }

//            if(ESP_OK == status) led.init();

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

        esp_err_t status{ESP_OK};

        //todo: need fixes to guarantee stable work
        if(state_e::STARTED == _state) {
//            led.blink(RGB::Led::GREEN, RGB::Led::NORMAL);
            bool provisioned = false;
            status |= wifi_prov_mgr_is_provisioned(&provisioned);
            if (!provisioned) {
                char service_name[24]{"AlcoTester-"};
                strcat(service_name, mac_addr_cstr);

                uint8_t custom_service_uuid[] = {
                        /* LSB <---------------------------------------
                         * ---------------------------------------> MSB */
                        0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                        0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
                };

                /* If your build fails with linker errors at this point, then you may have
                 * forgotten to enable the BT stack or BTDM BLE settings in the SDK (e.g. see
                 * the sdkconfig.defaults in the example project) */
                wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

                status |= wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, "YVyjPxHp", service_name,
                                                           nullptr);

                // Wait for service to complete
                wifi_prov_mgr_wait();
            }
            wifi_prov_mgr_deinit();

//            led.turn_of();
            ESP_LOGI(_log_tag, "%s: Waiting for state_mutex", __func__);
            std::lock_guard<std::mutex> state_guard(state_mutex);
            _state = state_e::READY_TO_CONNECT;
        }

        switch (_state) {
            case state_e::READY_TO_CONNECT: {
                ESP_LOGI(_log_tag, "%s: Waiting for connect_mutex", __func__);
                std::lock_guard<std::mutex> connect_guard(connect_mutex);

                ESP_LOGI(_log_tag, "%s: Calling esp_wifi_connect", __func__);
                status = esp_wifi_connect();
                ESP_LOGI(_log_tag, "%s: esp_wifi_connect: %s", __func__, esp_err_to_name(status));

                ESP_LOGI(_log_tag, "%s: Waiting for state_mutex", __func__);
                std::lock_guard<std::mutex> state_guard(state_mutex);
                if (ESP_OK == status)
                    _state = state_e::CONNECTING;
                break;
            }
            case state_e::STARTED:
            case state_e::CONNECTING:
            case state_e::WAITING_FOR_IP:
            case state_e::CONNECTED:
                break;
            case state_e::NOT_INITIALISED:
            case state_e::INITIALISED:
            case state_e::WAITING_FOR_CREDENTIALS:
            case state_e::DISCONNECTED:
            case state_e::ERROR:
                ESP_LOGE(_log_tag, "%s: Error state", __func__);
                status = ESP_FAIL;
                break;
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