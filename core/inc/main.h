//
// Created by notsure on 09.05.22.
//

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Gpio.h"

#define pdSECOND pdMS_TO_TICKS(1000)

class Main final {
public:
    esp_err_t setup(void);
    void loop(void);

    Gpio::GpioOutput blueLed{GPIO_NUM_5, false};
    Gpio::GpioOutput greenLed{GPIO_NUM_4, false};
};


#ifndef ALCOTESTER_MAIN_H
#define ALCOTESTER_MAIN_H

#endif //ALCOTESTER_MAIN_H
