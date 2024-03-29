//
// Created by notsure on 01.06.22.
//

#include "WebSocket.h"

WebSocket::state_e              WebSocket::state{WebSocket::state_e::DISCONNECTED};
esp_websocket_client_handle_t   WebSocket::client{};

void WebSocket::websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI("WS", "Connected to ws server");
            state = state_e::CONNECTED;

            char message[30];
            sprintf(message, R"({"macAdress":"%s"})", WIFI::Wifi::get_mac());
            ESP_LOGI("MAIN", "Sending message: %s", message);
            send(message);

            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI("WS", "Disconnected from ws server");
            break;
        case WEBSOCKET_EVENT_DATA: {

            ESP_LOGI("WS", "Recieved data from websocket");
            ESP_LOGI("WS", "op code: %i", data->op_code);
            if (data->op_code == 0x08 && data->data_len == 2) {
                ESP_LOGW("WS", "Received closed message with code=%d", 256*data->data_ptr[0] + data->data_ptr[1]);
            } else if (data->op_code == 0x0A) {
                char message[100];
                sprintf(message, R"({"macAdress":"%s","Misha":"privet","zdorov`ya":"pogibahym","some":"text"})", WIFI::Wifi::get_mac());
                ESP_LOGI("MAIN", "Sending message: %s", message);
                send(message);
            } else if (data->op_code == 0x01){
                ESP_LOGW("WS", "Received=%.*s", data->data_len, (char *)data->data_ptr);
                if(strncmp(data->data_ptr, R"({"command":"start"})", 19) == 0) {
                    state = state_e::WAITING_FOR_MESURE;
                }
            }

            break;
        }
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGI("WS", "WEBSOCKET_EVENT_ERROR");
            break;
    }
}

void WebSocket::ws_task(void *arg) {
    Sensor sensor = *static_cast<Sensor*>(arg);

    while (1) {
        if(state == state_e::WAITING_FOR_MESURE) {
            char message[42];
            sprintf(message, R"({"macAdress":"%s","value":%d})", WIFI::Wifi::get_mac(), sensor.measure());
            ESP_LOGI("MAIN", "Sending message: %s", message);
            send(message);

            state = state_e::CONNECTED;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t WebSocket::send(const char* message) {
    const int result = esp_websocket_client_send_text(client, message, strlen(message), portMAX_DELAY);
    return result > 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t WebSocket::init() {

    esp_err_t status{ESP_OK};

    client = esp_websocket_client_init(&config);

    status |= esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    if(ESP_OK != status) return status;
    status |= esp_websocket_client_start(client);

    if(ESP_OK != status) return status;
    status |= sensor.init();

    if(ESP_OK != status) return status;
    xTaskCreate(ws_task, "websocket", 4096, &sensor, 1, NULL);

    return status;
}