#pragma once

#include "hardware/sensors/sensor_id.h"
#include "util/circular_buffer.h"
#include "util/psram_allocator.h"
#include "util/semaphore_lockable.h"
#include <array>
#include <atomic>
#include <cmath>
#include <mutex>
#include <optional>
#include <type_traits>
#include <vector>

class sensor_definition_display
{
  public:
    constexpr sensor_definition_display(double range_min, double range_max, sensor_level level) noexcept
        : range_min{range_min}, range_max{range_max}, level{level}
    {
    }

    constexpr double get_range_min() const noexcept
    {
        return range_min;
    }

    constexpr double get_range_max() const noexcept
    {
        return range_max;
    }

    constexpr sensor_level get_level() const noexcept
    {
        return level;
    }

    constexpr bool is_in_range(double value) const noexcept
    {
        return range_min <= value && value < range_max;
    }

  public:
    const double range_min;
    const double range_max;
    const sensor_level level;
};

class sensor_definition
{
  public:
    constexpr sensor_definition(const std::string_view &name, const std::string_view &unit, const sensor_definition_display *display_definitions,
                                size_t display_definitions_count, float min_value, float max_value, float value_step) noexcept
        : name_{name}, unit_(unit), display_definitions_(display_definitions), display_definitions_count_(display_definitions_count),
          min_value_(min_value), max_value_(max_value), value_step_(value_step)
    {
    }

    constexpr sensor_level calculate_level(float value_p) const noexcept
    {
        for (uint8_t i = 0; i < display_definitions_count_; i++)
        {
            if (display_definitions_[i].is_in_range(value_p))
            {
                return display_definitions_[i].get_level();
            }
        }
        return sensor_level::no_level;
    }

    constexpr auto &&get_unit() const noexcept
    {
        return unit_;
    }
    constexpr auto &&get_name() const noexcept
    {
        return name_;
    }

    constexpr float get_max_value() const noexcept
    {
        return max_value_;
    }

    constexpr float get_min_value() const noexcept
    {
        return min_value_;
    }

    constexpr float get_value_step() const noexcept
    {
        return value_step_;
    }

  private:
    const std::string_view name_;
    const std::string_view unit_;
    const sensor_definition_display *display_definitions_;
    const uint8_t display_definitions_count_;
    const float min_value_;
    const float max_value_;
    const float value_step_;
};

/**
 * @class sensor_value
 * @brief Thread-safe wrapper for a floating-point sensor value.
 *
 * This class provides atomic access to a sensor value, allowing safe concurrent
 * reads and writes. It supports setting the value, marking it as invalid (using NAN),
 * and checking if the value has changed.
 *
 * Public Methods:
 * - float get_value() const: Returns the current sensor value.
 * - bool set_value(float value): Sets the sensor value and returns true if it changed.
 * - bool set_invalid_value(): Sets the sensor value to NAN, marking it as invalid.
 *
 * Private Members:
 * - std::atomic<float> value_: The atomic sensor value, initialized to NAN.
 *
 * Private Methods:
 * - bool set_value_(float value): Helper to set the value atomically and indicate change.
 */
class sensor_value
{
  public:
    float get_value() const
    {
        return value_.load();
    }

    bool set_value(float value)
    {
        return set_value_(value);
    }

    bool set_invalid_value()
    {
        return set_value_(NAN);
    }

  private:
    std::atomic<float> value_{NAN};

    /**
     * Returns true if changed
     */
    bool set_value_(float value)
    {
        if (value_.exchange(value) != value)
        {
            return true;
        }
        return false;
    }
};

template <uint16_t countT> class sensor_history_t
{
  public:
    typedef struct
    {
        float mean;
        float min;
        float max;
    } stats;

    using vector_history_t = std::vector<float, esp32::psram::allocator<float>>;

    typedef struct
    {
        std::optional<stats> stat;
        vector_history_t history;
    } sensor_history_snapshot;

    void add_value(float value)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        last_x_values_.push(value);
    }

    void clear()
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        last_x_values_.clear();
    }

    // Returns a snapshot of the last x values, grouped by `group_by_count`
    // If no values are present, returns an empty vector
    // If `group_by_count` is 0, returns all values
    sensor_history_snapshot get_snapshot(uint8_t group_by_count) const
    {
        vector_history_t return_values;

        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        const auto size = last_x_values_.size();
        if (size)
        {
            return_values.reserve(1 + (size / group_by_count));
            float value_max = std::numeric_limits<float>::min();
            float value_min = std::numeric_limits<float>::max();
            double sum = 0;
            double group_sum = 0;
            for (auto i = 0; i < size; i++)
            {
                const auto value = last_x_values_[i];
                sum += value;
                group_sum += value;
                value_max = std::max<float>(value, value_max);
                value_min = std::min<float>(value, value_min);

                if (((i + 1) % group_by_count) == 0)
                {
                    return_values.push_back(group_sum / (group_by_count));
                    group_sum = 0;
                }
            }

            // add partial group average
            if (size % group_by_count)
            {
                return_values.push_back(group_sum / (size % group_by_count));
            }

            stats stats_value;
            stats_value.max = value_max;
            stats_value.min = value_min;
            stats_value.mean = sum / size;
            return {stats_value, return_values};
        }
        else
        {
            return {std::nullopt, return_values};
        };
    }

    // Returns the average of the last x values
    // If no values are present, returns std::nullopt
    std::optional<float> get_average() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        const auto size = last_x_values_.size();
        if (size)
        {
            double sum = 0;
            for (auto i = 0; i < size; i++)
            {
                sum += last_x_values_[i];
            }
            return sum / size;
        }
        else
        {
            return std::nullopt;
        }
    }

  protected:
    // Calculates the linear trend (slope)
    // Returns a float representing the slope of the best-fit line.
    // A positive slope means increasing trend, negative means decreasing.
    float get_slope(size_t x) const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);

        const auto n_total = last_x_values_.size();
        if (n_total < 2 || x < 2)
        {
            return 0.0f;
        }

        const size_t n = std::min(x, n_total);
        const size_t offset = n_total - n;

        double sum_x = (n - 1) * n / 2.0;
        double sum_x2 = (n - 1) * n * (2 * n - 1) / 6.0;

        double sum_y = 0.0;
        double sum_xy = 0.0;

        float last_valid_value = 0.0f;
        bool has_valid = false;

        // Find first valid value
        for (size_t i = 0; i < n; ++i)
        {
            float val = last_x_values_[offset + i];
            if (!std::isnan(val))
            {
                last_valid_value = val;
                has_valid = true;
                break;
            }
        }

        if (!has_valid)
        {
            return 0.0f;
        }

        for (size_t i = 0; i < n; ++i)
        {
            const float raw = last_x_values_[offset + i];
            const float y = std::isnan(raw) ? last_valid_value : (last_valid_value = raw);

            sum_y += y;
            sum_xy += i * y;
        }

        double denominator = n * sum_x2 - sum_x * sum_x;
        return (denominator == 0.0) ? 0.0f : static_cast<float>((n * sum_xy - sum_x * sum_y) / denominator);
    }

  private:
    mutable esp32::semaphore data_mutex_;
    circular_buffer<float, countT> last_x_values_;
};

template <uint8_t reads_per_minuteT, uint16_t minutesT> class sensor_history_minute_t : public sensor_history_t<reads_per_minuteT * minutesT>
{
  public:
    static constexpr auto total_minutes = minutesT;
    static constexpr auto reads_per_minute = reads_per_minuteT;
    static constexpr auto sensor_interval = (60u * 1000 / reads_per_minute);

    float get_slope_per_minute(uint8_t last_minutes_to_consider) const
    {
        const auto slope = sensor_history_t<reads_per_minuteT * minutesT>::get_slope(reads_per_minuteT * last_minutes_to_consider);
        return (60u * slope) / sensor_interval;
    }
};

using sensor_history = sensor_history_minute_t<12, 720>;

constexpr std::array<sensor_definition_display, 0> no_level{};

constexpr std::array<sensor_definition, total_sensors> sensor_definitions{
    sensor_definition{"Humidity-1", "⁒", no_level.data(), no_level.size(), 0, 100, 1},
    sensor_definition{"Humidity-2", "⁒", no_level.data(), no_level.size(), 0, 100, 1},
    sensor_definition{"Humidity", "⁒", no_level.data(), no_level.size(), 0, 100, 1},
};

constexpr auto &&get_sensor_definition(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)];
}

constexpr auto &&get_sensor_name(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)].get_name();
}

constexpr auto &&get_sensor_unit(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)].get_unit();
}