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

//    status |= led.init();
    status |= wifi.init();

    if(ESP_OK == status) wifi.begin();

//    status |= sntp.init();

    status |= ws.init();

    while (ws.get_state() != WebSocket::state_e::CONNECTED);

    char message[30];
    sprintf(message, R"({"macAdress":"%s"})", WIFI::Wifi::get_mac());
    ESP_LOGI("MAIN", "Sending message: %s", message);
    ws.send(message);

//    led.blink(RGB::Led::PINK, RGB::Led::NORMAL);

    return status;
}

void Main::loop() {
    ESP_LOGI(LOG_TAG, "Hello World");
//    led.blink(RGB::Led::WHITE, RGB::Led::FAST);
//    vTaskDelay(5 * pdSECOND);
//    led.blink(RGB::Led::GREEN, RGB::Led::SLOW);
    vTaskDelay(5 * pdSECOND);
}
