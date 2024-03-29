#include "main.h"

#define LOG_TAG "MAIN"
#include "esp_log.h"

static Main my_main;

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG, "Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(LOG_TAG, "Initialising nvs memory");
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(my_main.setup());

    while (true) {
        my_main.loop();
    }
}

esp_err_t Main::setup() {
    esp_err_t status{ESP_OK};
    status |= wifi.init();

    if(ESP_OK == status) wifi.begin();

    status |= ws.init();

    return status;
}

void Main::loop() {
    vTaskDelay(5 * pdSECOND);
}
