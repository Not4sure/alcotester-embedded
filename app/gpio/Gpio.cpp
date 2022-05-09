//
// Created by notsure on 09.05.22.
//

#include "Gpio.h"

namespace Gpio {
    esp_err_t GpioBase::init() {
        esp_err_t status{ESP_OK};

        status |= gpio_config(&_cfg);
        return status;
    }

    esp_err_t GpioOutput::init() {
        esp_err_t status{GpioBase::init()};

        if (ESP_OK == status)
            status |= set(_inverted_logic);
        return status;
    }

    esp_err_t GpioOutput::set(const bool state) {
        _state = state;

        return gpio_set_level(_pin, _inverted_logic == !state);
    }
}
