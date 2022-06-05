//
// Created by notsure on 26.05.22.
//

#include "RGBLed.h"

namespace RGB {
    gpio_num_t red{GPIO_NUM_3};
    gpio_num_t green{GPIO_NUM_4};
    gpio_num_t blue{GPIO_NUM_5};
    bool inverted_logic{false};

    std::mutex Led::config_mtx{};
    uint8_t Led::red_duty{0};
    uint8_t Led::green_duty{0};
    uint8_t Led::blue_duty{0};
    uint16_t Led::delay{0};
    bool initialised{false};

    bool Led::ledc_callback_handler(const ledc_cb_param_t *param, void *user_arg) {
        if(LEDC_FADE_END_EVT == param->event) {
            std::lock_guard<std::mutex> guard(config_mtx);

            if(delay != 0) {
                const auto channel = static_cast<ledc_channel_t>(param->channel);
                const bool down = param->duty != 0;

                switch (channel) {
                    case LEDC_CHANNEL_0:
                        set_fade(channel, down ? 0 : red_duty, delay);
                        break;
                    case LEDC_CHANNEL_1:
                        set_fade(channel, down ? 0 : green_duty, delay);
                        break;
                    case LEDC_CHANNEL_2:
                        set_fade(channel, down ? 0 : blue_duty, delay);
                        break;
                    default:
                        break;
                }
            }
        }
        return true;
    }

    esp_err_t Led::blink(color_e color, speed_mode_e mode) {
        std::lock_guard<std::mutex> guard(config_mtx);

        delay = mode;
        red_duty = (color & 0xFF0000) >> 16;
        green_duty = (color & 0x00FFFF) >> 8;
        blue_duty = color & 0x0000FF;

        set_fade(LEDC_CHANNEL_0, red_duty, delay);
        set_fade(LEDC_CHANNEL_1, green_duty, delay);
        set_fade(LEDC_CHANNEL_2, blue_duty, delay);

        return ESP_OK;
    }

    void Led::set_fade(const ledc_channel_t channel, uint32_t duty, uint32_t delay) {
        ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,
                                channel, duty, delay);
        ledc_fade_start(LEDC_LOW_SPEED_MODE,
                        channel, LEDC_FADE_NO_WAIT);
    }

    esp_err_t Led::turn_of() {
        std::lock_guard<std::mutex> guard(config_mtx);

        delay = 0;
        return ESP_OK;
    }

    esp_err_t Led::_init() {
        esp_err_t status{ESP_OK};

        if(initialised) return status;

        const ledc_timer_config_t ledc_timer{
                .speed_mode = LEDC_LOW_SPEED_MODE,     // timer mode
                .duty_resolution = LEDC_TIMER_8_BIT,             // resolution of PWM duty
                .timer_num = LEDC_TIMER_0,              // timer index
                .freq_hz = 5000,                          // frequency of PWM signal
                .clk_cfg = LEDC_AUTO_CLK,                  // Auto select the source clock
        };

        status |= ledc_timer_config(&ledc_timer);
        if(ESP_OK != status) return status;

        ledc_channel_config_t red_config{
                .gpio_num   = red,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .channel    = LEDC_CHANNEL_0,
                .timer_sel  = LEDC_TIMER_0,
                .duty       = 0,
                .hpoint     = 0,
                .flags = {
                        .output_invert = static_cast<uint>(inverted_logic),
                },
        };

        status |= ledc_channel_config(&red_config);
        if(ESP_OK != status) return status;

        ledc_channel_config_t green_config{
                .gpio_num   = green,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .channel    = LEDC_CHANNEL_1,
                .timer_sel  = LEDC_TIMER_0,
                .duty       = 0,
                .hpoint     = 0,
                .flags = {
                        .output_invert = static_cast<uint>(inverted_logic),
                },
        };

        status |= ledc_channel_config(&green_config);
        if(ESP_OK != status) return status;

        ledc_channel_config_t blue_config{
                .gpio_num   = blue,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .channel    = LEDC_CHANNEL_2,
                .timer_sel  = LEDC_TIMER_0,
                .duty       = 0,
                .hpoint     = 0,
                .flags = {
                        .output_invert = static_cast<uint>(inverted_logic),
                },
        };

        status |= ledc_channel_config(&blue_config);
        if(ESP_OK != status) return status;

        status |= ledc_fade_func_install(0);
        if(ESP_OK != status) return status;

        ledc_cbs_t callbacks{
            .fade_cb = ledc_callback_handler
        };

        status |= ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, &callbacks, nullptr);
        if(ESP_OK != status) return status;

        status |= ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, &callbacks, nullptr);
        if(ESP_OK != status) return status;

        status |= ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, &callbacks, nullptr);
        if(ESP_OK != status) return status;

        if(ESP_OK == status) initialised = true;

        return status;
    }
}