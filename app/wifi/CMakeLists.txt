set(SOURCES ./Wifi.cpp ./Sntptime.cpp)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES freertos esp_wifi nvs_flash esp_event esp_netif lwip nvs32 wifi_provisioning rgbled)
