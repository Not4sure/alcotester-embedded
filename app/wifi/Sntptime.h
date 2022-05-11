//
// Created by notsure on 11.05.22.
//

#include <ctime>
#include <chrono>
#include <iomanip>

#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Wifi.h"

namespace SNTP {

    class Sntp final : private WIFI::Wifi {
    public:
        static Sntp &get_instance() {
            static Sntp sntp;
            return sntp;
        }

        enum time_source_e : uint8_t {
            TIME_SRC_UNKNOWN = 0,
            TIME_SRC_NTP = 1,
            TIME_SRC_GPS = 2,
            TIME_SRC_RADIO = 3,
            TIME_SRC_MANUAL = 4,
            TIME_SRC_ATOMIC_CLK = 5,
            TIME_SRC_CELL_NET = 6,
        };

        static esp_err_t init();

        static esp_err_t begin();

        [[nodiscard]] static auto time_point_now() noexcept { return std::chrono::system_clock::now(); }

        [[nodiscard]] static auto time_seance_last_update() noexcept { return time_point_now() - _last_update; }

        [[nodiscard]] static const char *ascii_time_now();

        [[nodiscard]] static std::chrono::seconds epoch_seconds() {
            return std::chrono::duration_cast<std::chrono::seconds>(time_point_now().time_since_epoch());
        }

    private:
        Sntp() = default;
        ~Sntp() { sntp_stop(); };

        constexpr static const char _time_zone[30] = {"GMT-3"};
//        constexpr static const char _time_zone[30] = {"GMT0BST,M3.5.0/1,M10.5.0/02"}; // London
        static bool _running;
        static time_source_e source;
        constexpr static char _log_tag[5] = {"Sntp"};

        static void callback_handler(timeval *tv);

        static std::chrono::_V2::system_clock::time_point _last_update;
    };
}