//
// Created by notsure on 04.06.22.
//
#pragma once

#include <esp_log.h>
#include <cmath>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

class Sensor {
    constexpr static const float clean_air_ratio = 4.4;
    static esp_adc_cal_characteristics_t adc_chars;

    float r0_value = 10;
    int rL_value = 1;

    esp_err_t calibrate();
    uint32_t voltage_to_ppm(const uint32_t voltage);
    uint32_t read_multisample(const int samples = 10);
public:
    esp_err_t init();
    uint16_t measure();
};
