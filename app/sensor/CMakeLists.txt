set(SOURCES ./Sensor.cpp)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_adc_cal freertos)
