//
// Created by notsure on 04.06.22.
//

#include "Sensor.h"

esp_adc_cal_characteristics_t Sensor::adc_chars{};

esp_err_t Sensor::init() {
    esp_err_t status{ESP_OK};

    if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
        ESP_LOGI("Sensor", "Will use burned default vRef");
    else if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
        ESP_LOGI("Sensor", "Will use burned default two point values");
    else
        ESP_LOGI("Sensor", "Will use default vref 1100");

    if(ESP_OK != status) return status;
    status |= adc1_config_width(ADC_WIDTH_BIT_12);

    if(ESP_OK != status) return status;
    status |= adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

    status |= calibrate();

    return status;
}

uint16_t Sensor::measure() {
    uint16_t max_reading{0};

    for (int i = 0; i < 20; i ++) {
        uint32_t adc_reading = read_multisample();

        ESP_LOGD("Sensor", "Reading %i value %4i", i, adc_reading);
        if(max_reading < adc_reading) max_reading = adc_reading;

        vTaskDelay(pdMS_TO_TICKS(250));
    }

    uint32_t voltage =  esp_adc_cal_raw_to_voltage(max_reading, &adc_chars);
    ESP_LOGI("Sensor", "Max voltage is %i", voltage);

        return voltage_to_ppm(voltage);
}

esp_err_t Sensor::calibrate() {
    uint32_t adc_reading = read_multisample();

    uint32_t voltage =  esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);

    float sensorResistance; //Define variable for sensor resistance

    sensorResistance = ((3.3 * rL_value * 1000) / voltage) - rL_value; //Calculate RS in fresh air
    if(sensorResistance < 0) return ESP_FAIL; //No negative values accepted.

    r0_value = sensorResistance/clean_air_ratio; //Calculate R0
    if(r0_value < 0) {
        r0_value = 0; //No negative values accepted.
        return ESP_FAIL;
    }

    return ESP_OK;
}

uint32_t Sensor::voltage_to_ppm(uint32_t voltage) {
    double sensorResistance{(((3.3 * rL_value * 1000) / voltage) - rL_value)};
    double ratio = sensorResistance / r0_value;

    ESP_LOGI("Sensor", "Ratio is %lf", ratio);

    double ppm = 393.4 * pow(ratio, -1.504);
    ESP_LOGI("Sensor", "ppm value is %lf", ppm);

    return (uint32_t)ppm;
}

uint32_t Sensor::read_multisample(int samples) {
    uint32_t adc_reading = 0;
    for (int j = 0; j < samples; j++) {
        adc_reading += adc1_get_raw(ADC1_CHANNEL_0);
    }
    return adc_reading / samples;
}