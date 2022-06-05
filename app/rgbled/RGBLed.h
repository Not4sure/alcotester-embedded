//
// Created by notsure on 26.05.22.
//

#pragma once

#include <mutex>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace RGB {
    class Led {
    private:
        static const gpio_num_t red;
        static const gpio_num_t green;
        static const gpio_num_t blue;
        static const bool inverted_logic;

//        const gpio_num_t red;
//        const gpio_num_t green;
//        const gpio_num_t blue;
//        const bool inverted_logic;

        static std::mutex config_mtx;
        static uint8_t red_duty;
        static uint8_t green_duty;
        static uint8_t blue_duty;
        static uint16_t delay;

        static bool initialised;

        static esp_err_t _init();

        static bool ledc_callback_handler(const ledc_cb_param_t *param, void *user_arg);

        static void set_fade(const ledc_channel_t channel, uint32_t duty, uint32_t delay);
    public:
//        Led(gpio_num_t red_pin, gpio_num_t green_pin, gpio_num_t blue_pin, bool inverted_logic = false):
//            red{red_pin}, green{green_pin}, blue{blue_pin}, inverted_logic{inverted_logic} {};

        enum color_e {
            WHITE = 0xFFFFFF,
            RED = 0xFF0000,
            GREEN = 0x00FF00,
            BLUE = 0x0000FF,
            ORANGE = 0xFF7E00,
            PINK = 0xFFC0CB,
        };

        enum speed_mode_e {
            FAST = 500,
            NORMAL = 1000,
            SLOW = 2000,
        };

        static esp_err_t init() { return _init(); };

        esp_err_t turn_of();

        esp_err_t blink(color_e color, speed_mode_e mode);

    };

}
