#pragma once

#include "hardware/sensors/ld2540/uart.h"
#include "target.h"
#include "util/static_queue.h"
#include "util/task_wrapper.h"
#include "zone.h"
#include <cmath>
#include <deque>
#include <map>
#include <span>
#include <string>

/**
 * @brief UART component responsible for processing the data stream provided by the HLK-LD2450 sensor
 */
class LD2450 : esp32::noncopyable
{
  public:
    LD2450() : uart_task_([this] { loop(); }), uart_tx_task_([this] { tx_task(); })
    {
    }
    constexpr static size_t target_count = 3;

    void init(const uart_init_config &init_config);

    /**
     * @brief Sets the x axis inversion flag
     * @param flip true if the x axis should be flipped, false otherwise
     */
    void set_flip_x_axis(bool flip)
    {
        flip_x_axis_ = flip;
    }

    /**
     * @brief Sets the fast of detection flag, which determines how the unoccupied state is determined.
     * @param value true if the x axis flipped
     */
    void set_fast_off_detection(bool value)
    {
        fast_off_detection_ = value;
    }

    /**
     * @brief Sets the maximum detection distance
     * @param distance maximum distance in meters
     */
    void set_max_distance(float distance)
    {
        if (!std::isnan(distance))
        {
            max_detection_distance_ = int(distance * 1000);
        }
    }

    /**
     * @brief Sets the maximum distance detection margin.
     * This margin is added to the max detection distance, such that detected targets still counts as present, even though they are outside of the max
     * detection distance. This can be used to reduce flickering.
     * @param distance margin distance in m
     */
    void set_max_distance_margin(float distance)
    {
        if (!std::isnan(distance))
            max_distance_margin_ = int(distance * 1000);
    }

    /**
     * @brief Gets the occupancy status of this LD2450 sensor.
     * @return true if at least one target is present, false otherwise
     */
    bool is_occupied() const
    {
        return is_occupied_;
    }

    /**
     * @brief Gets the specified target from this device.
     * @param i target index
     */
    const Target &get_target(size_t i) const
    {
        return targets_[i];
    }

    /**
     * @brief Restarts the sensor module
     */
    void perform_restart();

    /**
     * @brief Resets the module to it's factory default settings and performs a restart.
     */
    void perform_factory_reset();

    /**
     * @brief Set the sensors target tracking mode
     *
     * @param mode true for multi target mode, false for single target tracking mode
     */
    void set_tracking_mode(bool mode);

    /**
     * @brief Set the bluetooth state on the sensor
     *
     * @param state true if bluetooth should be enabled, false otherwise
     */
    void set_bluetooth_state(bool state);

    /**
     * @brief Requests the state of switches from the sensor.
     *
     */
    void read_switch_states();

  private:

    /**
     * @brief Reads and logs the sensors version number.
     */
    void log_sensor_version();



    /**
     * @brief Parses the input message and updates related components.
     * @param msg Message buffer
     * @param len Message content
     */
    void process_message(uint8_t *msg, int len);

    /**
     * @brief Parses the input configuration-message and updates related components.
     * @param msg Message buffer
     * @param len Message length
     */
    void process_config_message(const std::span<uint8_t> &msg);

    void write_command(const std::span<const uint8_t> &msg);

    void send_config_message(const std::span<const uint8_t>& command)
    {
        auto data = new std::vector<uint8_t>(command.begin(), command.end());
        command_queue_.enqueue(data, portMAX_DELAY);
    }

    /// @brief indicates whether the start sequence has been parsed
    uint8_t peek_status_ = 0;

    /// @brief Determines whether the x values are inverted
    bool flip_x_axis_ = false;

    /// @brief indicates whether a target is detected
    bool is_occupied_ = false;

    uint8_t target_count_{0};
    bool multi_tracking_state_{false};

    /// @brief Determines whether the fast unoccupied detection method is applied
    bool fast_off_detection_ = false;

    /// @brief Determines whether the sensor is in it's configuration mode
    // bool configuration_mode_ = false;

    /// @brief Indicated that the sensor is currently factory resetting
    // bool is_applying_changes_ = false;

    /// @brief Expected length of the configuration message
    int configuration_message_length_ = 0;

    /// @brief timestamp of the last message which was sent to the sensor
    // uint32_t command_last_sent_ = 0;

    /// @brief timestamp of the last received message
    // uint32_t last_message_received_ = 0;

    /// @brief timestamp at which the last available size change has occurred. Once the rx buffer has overflown it must be cleared manually on some
    /// configurations to receive new data
    // uint32_t last_available_change_ = 0;

    /// @brief timestamp of the last attempt to leave config mode if it's not responding
    // uint32_t last_config_leave_attempt_ = 0;

    /// @brief timestamp of lockout period after applying changes requiring a restart
    uint32_t apply_change_lockout_ = 0;

    /// @brief nr of available bytes during the last iteration
    // int last_available_size_ = 0;

    /// @brief Queue of commands to execute
    // std::deque<std::vector<uint8_t>> command_queue_;
    esp32::static_queue<std::vector<uint8_t> *, 128> command_queue_;

    /// @brief The maximum detection distance in mm
    int16_t max_detection_distance_ = 6000;

    /// @brief The margin added to the max detection distance in which a detect target still counts as present, even though it is outside of the
    /// max detection distance
    int16_t max_distance_margin_ = 250;

    /// @brief List of registered and mock tracking targets
    std::array<Target, target_count> targets_;

    /// @brief List of registered zones
    std::vector<Zone> zones_;

    uart_init_config uart_init_config_{};

    esp32::task uart_task_;
    esp32::task uart_tx_task_;
    uart uart_;

    void tx_task();
    
    void parse_rx_header();
    void process_rx();
    void loop();
    bool send_command_and_wait_ack(const std::span<const uint8_t>& command);
};