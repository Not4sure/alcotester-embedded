set(SOURCES ./WebSocket.cpp)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES freertos esp_websocket_client wifi sensor)