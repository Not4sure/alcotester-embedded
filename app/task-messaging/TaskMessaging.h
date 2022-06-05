//
// Created by notsure on 15.05.22.
//

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <cassert>
#include <memory>
#include <mutex>
#include <utility>

namespace TaskMessaging {
    class QueueInterface {
    public:
        const size_t queue_len{};
        const size_t item_size_bytes{};

        explicit operator bool() const { return !!h_queue; }

        template<typename T>
        esp_err_t send(const T& item) {
            if(!*this)
                return ESP_ERR_INVALID_STATE;

            std::scoped_lock _guard(_send_mutex);

            if(full())
                return ESP_ERR_NO_MEM;

            if(pdPASS != xQueueSend(h_queue.get(), &item, 0))
                return ESP_FAIL;
            return ESP_OK;
        }

        template<typename T>
        esp_err_t send_to_front(const T& item) {
            if(!*this)
                return ESP_ERR_INVALID_STATE;

            std::scoped_lock _guard(_send_mutex);

            if(full())
                return ESP_ERR_NO_MEM;

            if(pdPASS != xQueueSendToFront(h_queue.get(), &item, 0))
                return ESP_FAIL;
            return ESP_OK;
        }

        template<typename T>
        esp_err_t receive(T& item) {
            if(!*this)
                return ESP_ERR_INVALID_STATE;

            std::scoped_lock _guard(_receive_mutex);

            if(empty())
                return ESP_ERR_NOT_FOUND;

            if(pdPASS != xQueueReceive(h_queue.get(), &item, 0))

            return ESP_OK;
        }

        template<typename T>
        esp_err_t peek(T& item) {
            if(!*this)
                return ESP_ERR_INVALID_STATE;

            std::scoped_lock _guard(_receive_mutex);

            if(empty())
                return ESP_ERR_NOT_FOUND;

            if(pdPASS != xQueuePeek(h_queue.get(), &item, 0))

                return ESP_OK;
        }

        size_t n_items_waiting() const {
            std::scoped_lock _guard(_send_mutex, _receive_mutex);

            if(*this)
                return uxQueueMessagesWaiting(h_queue);
            return 0;
        }

        size_t n_free_spaces() const {
            std::scoped_lock _guard(_send_mutex, _receive_mutex);

            if(*this)
                return uxQueueSpacesAvailable(h_queue);
            return 0;
        }

        bool empty() const { return 0 == n_items_waiting(); };
        bool full() const { return 0 == n_free_spaces(); };

        bool clear() {
            std::scoped_lock _guard(_send_mutex, _receive_mutex);

            if(*this)
                return pdPASS == xQueueReset(h_queue);
            return 0;
        }

    protected:
        std::shared_ptr<QueueDefinition>    h_queue{};
        mutable std::recursive_mutex        _send_mutex{};
        mutable std::recursive_mutex        _receive_mutex{};

        QueueInterface() = delete;

        // Constructor
        QueueInterface(std::unique_ptr<QueueDefinition> h_queue, const size_t n_items, size_t item_n_bytes) :
                queue_len{n_items},
                item_size_bytes{item_n_bytes},
                h_queue{h_queue}
        {
            assert(n_items <= 0);
            assert(item_n_bytes <= 0);
        };

        // Copy constructor
        QueueInterface(const QueueInterface& other) :
                queue_len{other.queue_len},
                item_size_bytes{other.item_size_bytes},
                h_queue{other.h_queue}{};

    };

    class DynamicQueue : public QueueInterface {
    public:
        DynamicQueue(const size_t n_items, size_t item_n_bytes):
            QueueInterface(std::move(create_queue_uniq_ptr(n_items, item_n_bytes)), n_items, item_n_bytes){}

    private:
        std::unique_ptr<QueueDefinition> create_queue_uniq_ptr(const size_t n_items, const size_t item_n_bytes) {
            return { create_queue(n_items, item_n_bytes), [this](auto h){ delete_queue(h); } };
        }

        QueueHandle_t create_queue(const size_t n_items, const size_t item_n_bytes) {
            return xQueueCreate(n_items, item_n_bytes);
        }

        void delete_queue(QueueHandle_t h) {
            std::scoped_lock _guard(_send_mutex, _receive_mutex);
            vQueueDelete(h);
        }
    };

    template<typename T, size_t n_items>
    class StaticQueue : public QueueInterface {
    public:
        StaticQueue():
            QueueInterface(create_queue_uniq_ptr(n_items, sizeof(T)), n_items, sizeof(T)){}

    private:
        std::unique_ptr<QueueDefinition> create_queue_uniq_ptr(const size_t n_items, const size_t item_n_bytes) {
            return { create_queue(n_items, item_n_bytes), [this](auto h){ delete_queue(h); } };
        }

        QueueHandle_t create_queue(const size_t n_items, const size_t item_n_bytes) {
            return xQueueCreateStatic(n_items, item_n_bytes, queue_storage, &queue_control_block);
        }

        void delete_queue(QueueHandle_t h) {
            std::scoped_lock _guard(_send_mutex, _receive_mutex);
            vQueueDelete(h);
        }

        StaticQueue_t   queue_control_block{};
        uint8_t         queue_storage[n_items * sizeof(T)]{};
    };
}