//
// Created by notsure on 09.05.22.
//

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_event.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "Wifi.h"
#include "Gpio.h"
#include "Sntptime.h"
#include "Nvs32.h"

#define pdSECOND pdMS_TO_TICKS(1000)

class Main final {
public:
    Main(): sntp{SNTP::Sntp::get_instance()} {}

    esp_err_t setup();
    void loop();

    Gpio::GpioOutput led{GPIO_NUM_4};
    WIFI::Wifi wifi;
    SNTP::Sntp& sntp;
};
