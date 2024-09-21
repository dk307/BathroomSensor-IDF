#include "hardware/sensors/ld2540/ld2450.h"
#include "logging/logging_tags.h"
#include "util/cores.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include "util/misc.h"
#include <esp_log.h>
#include <mutex>

#define POST_RESTART_LOCKOUT_DELAY 2000

#define COMMAND_MAX_RETRIES 10
#define COMMAND_RETRY_DELAY 100
#define COMMAND_TIMEOUT 2000

#define COMMAND_ENTER_CONFIG 0xFF
#define COMMAND_LEAVE_CONFIG 0xFE
#define COMMAND_READ_VERSION 0xA0
#define COMMAND_RESTART 0xA3
#define COMMAND_FACTORY_RESET 0xA2

#define COMMAND_READ_TRACKING_MODE 0x91
#define COMMAND_SINGLE_TRACKING_MODE 0x80
#define COMMAND_MULTI_TRACKING_MODE 0x90

#define COMMAND_READ_MAC 0xA5
#define COMMAND_BLUETOOTH 0xA4

#define COMMAND_SET_BAUD_RATE 0xA1

void LD2450::init(const uart_init_config &init_config)
{
    uart_init_config_ = init_config;
    uint8_t i = 1;
    for (auto &&target : targets_)
    {
        // Generate Names if not present
        if (target.get_name().empty())
        {
            const std::string name = std::string("Target ").append(esp32::string::to_string(i));
            target.set_name(name);
        }
        i++;

        target.set_fast_off_detection(fast_off_detection_);
    }

    // start task
    CHECK_THROW_ESP(uart_task_.spawn_pinned("ld2450", 1024 * 4, esp32::task::default_priority, esp32::hardware_core));
}

void LD2450::tx_task()
{
    ESP_LOGI(UART_TAG, "Start to uart tx task on core:%d", xPortGetCoreID());

    bool configuration_mode = false;
    constexpr uint8_t enter_command_mode[4] = {COMMAND_ENTER_CONFIG, 0x00, 0x01, 0x00};
    constexpr uint8_t leave_command_mode[2] = {COMMAND_LEAVE_CONFIG, 0x00};

    while (true)
    {
        std::vector<uint8_t> *command = nullptr;
        if (command_queue_.peek(command, 0))
        {
            if (!configuration_mode)
            {
                send_command_and_wait_ack(enter_command_mode);
                configuration_mode = true;
            }

            send_command_and_wait_ack(*command);

            // remove from queue
            command_queue_.dequeue(command, 0);

            if ((command->front() == COMMAND_RESTART) || (command->front() == COMMAND_FACTORY_RESET))
            {
                configuration_mode = false;
            }

            delete command;
        }
        else if (configuration_mode)
        {
            send_command_and_wait_ack(leave_command_mode);
            configuration_mode = false;
        }
        else
        {
            command_queue_.peek(command, portMAX_DELAY);
        }
    }

    vTaskDelete(NULL);
}

bool LD2450::send_command_and_wait_ack(const std::span<const uint8_t> &command)
{
    write_command(command);

    // wait for the command ack
    uint32_t notification_value = 0;
    const auto result = xTaskNotifyWait(pdFALSE,             /* Don't clear bits on entry. */
                                        ULONG_MAX,           /* Clear all bits on exit. */
                                        &notification_value, /* Stores the notified value. */
                                        pdMS_TO_TICKS(COMMAND_TIMEOUT));

    if (result == pdPASS)
    {
        if (notification_value == command.front())
        {
            // wait for the command to finish
            switch (command[0])
            {
            case COMMAND_RESTART:
                vTaskDelay(pdMS_TO_TICKS(POST_RESTART_LOCKOUT_DELAY));
                break;
            case COMMAND_FACTORY_RESET:
                vTaskDelay(pdMS_TO_TICKS(POST_RESTART_LOCKOUT_DELAY));
                break;
            }

            return true;
        }
        else
        {
            ESP_LOGE(UART_TAG, "Notification returned message type:%lu when expected :%u", notification_value, command.front());
            return false;
        }
    }
    else
    {
        ESP_LOGE(UART_TAG, "Timed out for notification:%d", command.front());
        return false;
    }
}

void LD2450::loop()
{
    ESP_LOGI(UART_TAG, "Start to uart task on core:%d", xPortGetCoreID());

    // set up uart & queue
    uart_.init(uart_init_config_);

    ESP_LOGI(UART_TAG, "UART Init Done");

    // Acquire current switch states and update related components
    read_switch_states();
    log_sensor_version();

    // start command tx task
    CHECK_THROW_ESP(uart_task_.spawn_pinned("ld2450_tx", 1024 * 4, esp32::task::default_priority, esp32::hardware_core));

    // this task is used as rx task
    uart_event_t event;
    while (true)
    {
        // Waiting for UART event.
        if (xQueueReceive(uart_.get_queue_handle(), reinterpret_cast<void *>(&event), portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
                ESP_LOGI(UART_TAG, "Rx data size: %d", event.size);
                process_rx();
                break;
            case UART_FIFO_OVF:
                ESP_LOGI(UART_TAG, "hw fifo overflow");
                uart_.clear();
                break;
            case UART_BUFFER_FULL:
                ESP_LOGI(UART_TAG, "ring buffer full");
                uart_.clear();
                break;
            case UART_BREAK:
                ESP_LOGI(UART_TAG, "uart rx break");
                break;
            case UART_PARITY_ERR:
                ESP_LOGI(UART_TAG, "uart parity error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGI(UART_TAG, "uart frame error");
                break;
            default:
                ESP_LOGI(UART_TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }

    vTaskDelete(NULL);
}

void LD2450::process_rx()
{
    bool processed_message = false;
    do
    {
        processed_message = false;

        parse_rx_header();

        if (peek_status_ == 1 && uart_.available() >= 26)
        {
            uint8_t msg[26] = {0x00};
            uart_.read_array(msg);
            peek_status_ = 0;

            // Skip invalid messages
            if (msg[24] != 0x55 || msg[25] != 0xCC)
                return;

            process_message(msg, 24);
            processed_message = true;
        }

        if (peek_status_ == 2 && (uart_.available() >= 2 || configuration_message_length_ > 0))
        {
            if (configuration_message_length_ == 0)
            {
                // Read message content length
                uint8_t content_length[2];
                uart_.read_array(content_length);
                configuration_message_length_ = content_length[1] << 8 | content_length[0];
                // Limit max message length
                configuration_message_length_ = std::min(configuration_message_length_, 20);
            }

            // Wait until message and frame end are available
            if (uart_.available() >= configuration_message_length_ + 4)
            {
                uint8_t msg[configuration_message_length_ + 4] = {0x00};
                uart_.read_array(std::span<uint8_t>(msg, configuration_message_length_ + 4));

                // Assert frame end read correctly
                if (msg[configuration_message_length_] == 0x04 && msg[configuration_message_length_ + 1] == 0x03 &&
                    msg[configuration_message_length_ + 2] == 0x02 && msg[configuration_message_length_ + 3] == 0x01)
                {
                    process_config_message( std::span<uint8_t>(msg, configuration_message_length_));
                }
                configuration_message_length_ = 0;
                peek_status_ = 0;
                processed_message = true;
            }
        }

    } while (processed_message);
}

void LD2450::parse_rx_header()
{
    constexpr uint8_t update_header[4] = {0xAA, 0xFF, 0x03, 0x00};
    constexpr uint8_t config_header[4] = {0xFD, 0xFC, 0xFB, 0xFA};

    // Skip stream until start of message and parse header
    while (!peek_status_ && uart_.available() >= 4)
    {
        // Try to read the header and abort on mismatch
        const uint8_t *header;
        bool skip = false;
        uint8_t message_type{};
        uint8_t first_byte = uart_.read_byte();
        if (first_byte == update_header[0])
        {
            header = update_header;
            message_type = 1;
        }
        else if (first_byte == config_header[0])
        {
            header = config_header;
            message_type = 2;
        }
        else
        {
            skip = true;
        }

        for (int i = 1; i < 4 && !skip; i++)
        {
            if (uart_.read_byte() != header[i])
                skip = true;
        }

        if (!skip)
            // Flag successful header reading
            peek_status_ = message_type;
    }
}

void LD2450::process_message(uint8_t *msg, int len)
{
    // last_message_received_ = esp32::millis();
    // configuration_mode_ = false;

    for (int i = 0; i < targets_.size(); i++)
    {
        int offset = 8 * i;

        int16_t x = msg[offset + 1] << 8 | msg[offset + 0];
        if (msg[offset + 1] & 0x80)
            x = -x + 0x8000;
        int16_t y = (msg[offset + 3] << 8 | msg[offset + 2]);
        if (y != 0)
            y -= 0x8000;
        int16_t speed = msg[offset + 5] << 8 | msg[offset + 4];
        if (msg[offset + 5] & 0x80)
            speed = -speed + 0x8000;
        int16_t distance_resolution = msg[offset + 7] << 8 | msg[offset + 6];

        // Flip x axis if required
        x = x * (flip_x_axis_ ? -1 : 1);

        targets_[i].update_values(x, y, speed, distance_resolution);

        // Filter targets further than max detection distance
        if (y <= max_detection_distance_ || (targets_[i].is_present() && y <= max_detection_distance_ + max_distance_margin_))
            targets_[i].update_values(x, y, speed, distance_resolution);
        else if (y >= max_detection_distance_ + max_distance_margin_)
            targets_[i].clear();
    }

    uint8_t target_count = 0;
    for (auto &&target : targets_)
    {
        target_count += target.is_present();
    }
    is_occupied_ = target_count > 0;
    target_count_ = target_count;

    // Update zones and related components
    for (auto &&zone : zones_)
    {
        zone.update_from_targets();
    }
}

void LD2450::process_config_message(const std::span<uint8_t> &msg)
{
    // Remove command from Queue upon receiving acknowledgement
    xTaskNotify(uart_tx_task_.handle(), msg.front(), eSetValueWithoutOverwrite);

    if (msg[0] == COMMAND_READ_VERSION && msg[1] == true)
    {
        ESP_LOGI(UART_TAG, "Sensor Firmware-Version: V%X.%02X.%02X%02X%02X%02X", msg[7], msg[6], msg[11], msg[10], msg[9], msg[8]);
    }

    if (msg[0] == COMMAND_READ_MAC && msg[1] == true)
    {

        bool bt_enabled = !(msg[4] == 0x08 && msg[5] == 0x05 && msg[6] == 0x04 && msg[7] == 0x03 && msg[8] == 0x02 && msg[9] == 0x01);
        if (bt_enabled)
        {
            ESP_LOGI(UART_TAG, "Sensor MAC-Address: %02X:%02X:%02X:%02X:%02X:%02X", msg[4], msg[5], msg[6], msg[7], msg[8], msg[9]);
        }
        else
        {
            ESP_LOGI(UART_TAG, "Sensor MAC-Address: Bluetooth disabled!");
        }
    }

    if (msg[0] == COMMAND_READ_TRACKING_MODE && msg[1] == true)
    {
        multi_tracking_state_ = msg[4] == 0x02;
    }
}

void LD2450::log_sensor_version()
{
    const uint8_t read_version[2] = {COMMAND_READ_VERSION, 0x00};
    send_config_message(read_version);
}

void LD2450::perform_restart()
{
    const uint8_t restart[2] = {COMMAND_RESTART, 0x00};
    send_config_message(restart);
    read_switch_states();
}

void LD2450::perform_factory_reset()
{
    const uint8_t reset[2] = {COMMAND_FACTORY_RESET, 0x00};
    send_config_message(reset);
    perform_restart();
}

void LD2450::set_tracking_mode(bool mode)
{
    if (mode)
    {
        const uint8_t set_tracking_mode[2] = {COMMAND_MULTI_TRACKING_MODE, 0x00};
        send_config_message(set_tracking_mode);
    }
    else
    {
        const uint8_t set_tracking_mode[2] = {COMMAND_SINGLE_TRACKING_MODE, 0x00};
        send_config_message(set_tracking_mode);
    }

    const uint8_t request_tracking_mode[2] = {COMMAND_READ_TRACKING_MODE, 0x00};
    send_config_message(request_tracking_mode);
}

void LD2450::set_bluetooth_state(bool state)
{
    const uint8_t set_bt[4] = {COMMAND_BLUETOOTH, 0x00, state, 0x00};
    send_config_message(set_bt);
    perform_restart();
}

void LD2450::read_switch_states()
{
    const uint8_t request_tracking_mode[2] = {COMMAND_READ_TRACKING_MODE, 0x00};
    send_config_message(request_tracking_mode);
}

void LD2450::write_command(const std::span<const uint8_t> &msg)
{
    constexpr std::array<uint8_t, 4> header{0xFD, 0xFC, 0xFB, 0xFA};
    constexpr std::array<uint8_t, 4> footer{0x04, 0x03, 0x02, 0x01};

    // Write frame header
    uart_.write_array(header);

    // Write message length
    uart_.write_byte(static_cast<uint8_t>(msg.size()));
    uart_.write_byte(static_cast<uint8_t>(msg.size() >> 8));

    // Write message content
    uart_.write_array(msg);

    // Write frame end
    uart_.write_array(footer);

    uart_.flush();
}
