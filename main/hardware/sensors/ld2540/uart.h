#pragma once
#include "sdkconfig.h"
#include "util/singleton.h"
#include <array>
#include <cstring>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <optional>
#include <span>
#include <vector>

typedef struct
{
    uart_port_t uart_port_;
    gpio_num_t tx_pin_;
    gpio_num_t rx_pin_;
    size_t rx_buffer_size_;
    uint32_t baud_rate_;
} uart_init_config;

class uart : public esp32::noncopyable
{
  public:
    ~uart();
    void init(const uart_init_config &config);

    void write_byte(uint8_t data);
    void write_array(const std::span<const uint8_t> &data);

    uint8_t read_byte();
    void read_array(const std::span<uint8_t> &buffer);

    size_t available();
    void flush();

    void clear();

    const QueueHandle_t &get_queue_handle() const
    {
        return uart_event_queue_;
    }

  private:
    uart_config_t get_config(uint32_t baud_rate);

    uart_port_t uart_port_{};
    QueueHandle_t uart_event_queue_{};
};
