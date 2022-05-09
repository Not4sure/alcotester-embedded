#include "main.h"

#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#include "esp_log.h"

#define LOG_TAG "MAIN"
#define pdSECOND pdMS_TO_TICKS(1000)

static Main my_main;

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(my_main.setup());

    while (true) {
        my_main.loop();
    }
}

esp_err_t Main::setup() {
    esp_err_t status{ESP_OK};

    status |= blueLed.init();
    status |= greenLed.init();
    return status;
}

void Main::loop() {
    greenLed.set(true);
    blueLed.set(true);
    vTaskDelay(pdSECOND);
    greenLed.set(false);
    blueLed.set(false);
    vTaskDelay(pdSECOND);
}
