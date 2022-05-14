//
// Created by notsure on 11.05.22.
//

#pragma once

#include <cstring>
#include <esp_log.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"

namespace NVS {
    class Nvs {
        const char* _log_tag{nullptr};
        const char* partition_name{nullptr};
        nvs_handle_t handle{};

    public:
        constexpr Nvs(const char* partition_name = "nvs"):
            _log_tag{partition_name},
            partition_name{partition_name} {};

        [[nodiscard]] esp_err_t init() { return _open(partition_name, handle); }

        [[nodiscard]] esp_err_t get_size(const char* key, size_t& size) const
            { return nvs_get_blob(handle, key, nullptr, &size); }

        template<typename T>
        [[nodiscard]] esp_err_t get(const char* key, T* output, size_t length = 1)
            { return _get_buffer(handle, key, output, length); }

        template<typename T>
        [[nodiscard]] esp_err_t set(const char* key, const T* input, size_t length = 1)
            { return _set_buffer(handle, key, input, length); }

        template<typename T>
        [[nodiscard]] esp_err_t verify(const char* key, const T* input, size_t length = 1)
            { return _verify_buffer(handle, key, input, length); }

    private:
        [[nodiscard]] static esp_err_t _open(const char* const partition_name, nvs_handle_t& handle, const nvs_open_mode_t mode = NVS_READWRITE)
            { return nvs_open(partition_name, mode, &handle); };

        template<typename T>
        [[nodiscard]] static esp_err_t _get_buffer(nvs_handle_t handle, const char* key, T* output, size_t& len) {
            size_t n_bytes{sizeof(T) * len};

            esp_err_t status{ESP_OK};

            if(nullptr == key || 0 == strlen(key) || nullptr == output || 0 >= len)
                status |= ESP_ERR_INVALID_ARG;

            if(ESP_OK == status)
                status |= nvs_get_blob(handle, key, output, &n_bytes);

            if(ESP_OK != status)
                ESP_LOGW("NVS", "%s\t%u%u", esp_err_to_name(status), len, n_bytes);
            len = n_bytes / sizeof(T);

            return status;
        }

        template<typename T>
        [[nodiscard]] static esp_err_t _set_buffer(nvs_handle_t handle, const char* key, const T* input, size_t len) {
            esp_err_t status{ESP_OK};

            if(nullptr == key || 0 == strlen(key) || nullptr == input || 0 >= len)
                status |= ESP_ERR_INVALID_ARG;

            if(ESP_OK == status)
                status |= nvs_set_blob(handle, key, input, sizeof(T) * len);

            if(ESP_OK == status)
                status |= nvs_commit(handle);

            if(ESP_OK == status)
                status |= _verify_buffer(handle, key, input, len);

            return status;
        }

        template<typename T>
        [[nodiscard]] static esp_err_t _verify_buffer(nvs_handle_t handle, const char* key, const T* input, size_t len) {
            T* buf_in_nvs = new T[len]{};
            size_t n_items_in_nvs = len;

            if(nullptr == buf_in_nvs)
                return ESP_ERR_NO_MEM;

            esp_err_t status{_get_buffer(handle, key, buf_in_nvs, n_items_in_nvs)};

            if(ESP_OK == status)
                if(n_items_in_nvs != len)
                    status |= ESP_ERR_NVS_INVALID_LENGTH;

            if(ESP_OK == status)
                if(0 != memcmp(input, buf_in_nvs, len * sizeof(T)))
                    status = ESP_FAIL;

            delete[] buf_in_nvs;
            return status;
        }
    };
}