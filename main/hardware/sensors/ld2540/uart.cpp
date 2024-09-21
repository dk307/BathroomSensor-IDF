#include "hardware/sensors/ld2540/uart.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include <esp_log.h>
#include <mutex>

uart::~uart()
{
}

uart_config_t uart::get_config(uint32_t baud_rate)
{
    uart_config_t uart_config{};
    uart_config.baud_rate = baud_rate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    uart_config.rx_flow_ctrl_thresh = 122;

    return uart_config;
}

void uart::init(const uart_init_config &init_config)
{
    uart_port_ = init_config.uart_port_;
    ESP_LOGI(UART_TAG, "Setting up UART %u", init_config.uart_port_);
    const uart_config_t uart_config = get_config(init_config.baud_rate_);
    CHECK_THROW_ESP(uart_param_config(init_config.uart_port_, &uart_config));
    CHECK_THROW_ESP(uart_set_line_inverse(init_config.uart_port_, 0));
    CHECK_THROW_ESP(uart_set_pin(init_config.uart_port_, init_config.tx_pin_, init_config.rx_pin_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    CHECK_THROW_ESP(uart_driver_install(init_config.uart_port_, init_config.rx_buffer_size_, 0, 64, &uart_event_queue_, 0));
}

void uart::write_byte(uint8_t data)
{
    const uint8_t data_array[] = {data};
    write_array(data_array);
}

void uart::write_array(const std::span<const uint8_t> &data)
{
    const auto written = uart_write_bytes(uart_port_, data.data(), data.size());
    if (written == -1)
    {
        CHECK_THROW_ESP2(ESP_FAIL, "Failed to write data to uart");
    }
    else if (written != data.size())
    {
        CHECK_THROW_ESP2(ESP_FAIL, "Failed to write complete data to uart");
    }
}

uint8_t uart::read_byte()
{
    uint8_t data[1]{};
    read_array(data);
    return data[0];
}

void uart::read_array(const std::span<uint8_t> &buffer)
{
    const auto read = uart_read_bytes(uart_port_, buffer.data(), buffer.size(), portMAX_DELAY);
    if (read == -1)
    {
        CHECK_THROW_ESP2(ESP_FAIL, "Failed to read data from uart");
    }
    else if (read != buffer.size())
    {
        CHECK_THROW_ESP2(ESP_FAIL, "Failed to read complete data from uart");
    }
}

size_t uart::available()
{
    size_t available{};
    CHECK_THROW_ESP(uart_get_buffered_data_len(uart_port_, &available));
    return available;
}

void uart::flush()
{
    ESP_LOGD(UART_TAG, "Flushing");
    CHECK_THROW_ESP(uart_wait_tx_done(uart_port_, portMAX_DELAY));
}

void uart::clear()
{
    ESP_LOGI(UART_TAG, "Clearing queue");
    CHECK_THROW_ESP(uart_flush_input(uart_port_));
    xQueueReset(uart_event_queue_);
}
