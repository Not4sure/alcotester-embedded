//
// Created by notsure on 11.05.22.
//

#include "Sntptime.h"

#include "esp_log.h"

namespace SNTP {

    std::chrono::_V2::system_clock::time_point _last_update{};
    Sntp::time_source_e Sntp::source{Sntp::time_source_e::TIME_SRC_UNKNOWN};
    bool Sntp::_running = false;

    void Sntp::callback_handler(timeval *tv) {
        ESP_LOGD(_log_tag, "Time Update %s", ascii_time_now());
    }

    const char *Sntp::ascii_time_now() {
        const std::time_t time_now{std::chrono::system_clock::to_time_t(time_point_now())};

        return std::asctime(std::localtime(&time_now));
    }

    esp_err_t Sntp::init() {

        if (!_running) {
            ESP_LOGI(_log_tag, "%s: Waiting for connected state", __func__);
            while (state_e::CONNECTED != Wifi::get_state())
                vTaskDelay(pdMS_TO_TICKS(1000));

            setenv("TZ", _time_zone, 1);
            tzset();

            sntp_setoperatingmode(SNTP_OPMODE_POLL);

            sntp_setservername(0, "time.google.com");
            sntp_setservername(1, "pool.ntp.com");

            sntp_set_time_sync_notification_cb(&callback_handler);
            sntp_set_sync_interval(20 * 60 * 1000);

            sntp_init();

            source = TIME_SRC_NTP;

            ESP_LOGI(_log_tag, "%s: Initialised", __func__ );
            _running = true;
        }

        if (_running)
            return ESP_OK;
        return ESP_FAIL;
    }

    esp_err_t Sntp::begin() {
        esp_err_t status{ESP_OK};

        return status;
    }

}