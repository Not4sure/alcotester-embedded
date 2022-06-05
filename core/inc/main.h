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
#include "Sntptime.h"
#include "Nvs32.h"
#include "WebSocket.h"

//#include "RGBLed.h"

#define pdSECOND pdMS_TO_TICKS(1000)

class Main final {
public:
    Main(): sntp{SNTP::Sntp::get_instance()} {}

    esp_err_t setup();
    void loop();

    WIFI::Wifi wifi;
    SNTP::Sntp& sntp;
    WebSocket ws{"ws://evening-brook-47964.herokuapp.com", 80};
//    RGB::Led led{GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5};
};
