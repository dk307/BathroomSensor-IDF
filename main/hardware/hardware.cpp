#include "hardware/hardware.h"
#include "app_events.h"
#include "config/config_manager.h"
#include "hardware/display/display.h"
#include "homekit/homekit_integration.h"
#include "logging/logging_tags.h"
#include "util/cores.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include "util/misc.h"
#include <driver/i2c.h>
#include <esp_log.h>

template <class T> bool hardware::read_sensor_if_time(T &sensor, uint64_t &last_read)
{
    bool updated = false;
    const auto now = esp32::millis();
    if (now - last_read >= sensor_history::sensor_interval)
    {
        for (auto &&value : sensor.read())
        {
            set_sensor_value(std::get<0>(value), std::get<1>(value));
            updated = true;
        }
        last_read = now;
    }
    return updated;
}

float hardware::get_sensor_value(sensor_id_index index) const
{
    auto &&sensor = get_sensor(index);
    return sensor.get_value();
}

sensor_history::sensor_history_snapshot hardware::get_sensor_detail_info(sensor_id_index index)
{
    return (*sensors_history_)[static_cast<size_t>(index)].get_snapshot(sensor_history::reads_per_minute);
}

void hardware::begin()
{
    CHECK_THROW_ESP(i2cdev_init());
    sensor_refresh_task_.spawn_pinned("sensor_task", 4 * 1024, esp32::task::default_priority, esp32::hardware_core);
}

void hardware::set_sensor_value(sensor_id_index index, float value)
{
    bool changed;
    const auto i = static_cast<size_t>(index);
    if (!std::isnan(value))
    {
        (*sensors_history_)[i].add_value(value);
        changed = sensors_[i].set_value(value);
        ESP_LOGI(HARDWARE_TAG, "Updated for sensor:%.*s Value:%g", get_sensor_name(index).size(), get_sensor_name(index).data(),
                 sensors_[i].get_value());
    }
    else
    {
        ESP_LOGW(HARDWARE_TAG, "Got an invalid value for sensor:%.*s", get_sensor_name(index).size(), get_sensor_name(index).data());
        (*sensors_history_)[i].clear();
        changed = sensors_[i].set_invalid_value();
    }

    if (changed)
    {
        CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, SENSOR_VALUE_CHANGE, index));
    }
}

void hardware::sensor_task_ftn()
{
    try
    {
        ESP_LOGI(HARDWARE_TAG, "Sensor task started on core:%d", xPortGetCoreID());

        led_.init();

        // light green on start
        led_.set_color(0, 64, 0);

        TickType_t initial_delay = 0;

        initial_delay = std::max<TickType_t>(initial_delay, sht3x_sensor1_.get_initial_delay());
        initial_delay = std::max<TickType_t>(initial_delay, sht3x_sensor2_.get_initial_delay());

        sht3x_sensor1_.init(I2C_NUM_0, GPIO_NUM_42, GPIO_NUM_2);
        sht3x_sensor2_.init(I2C_NUM_1, GPIO_NUM_38, GPIO_NUM_39);

        const uart_init_config ld2450_init_config{UART_NUM_0, GPIO_NUM_47, GPIO_NUM_21, 4 * 1024, 256000};
        ld2450_.init(ld2450_init_config);

        // Wait until all sensors are ready
        vTaskDelay(initial_delay);

        // clear on no error
        led_.clear();

        ESP_LOGI(HARDWARE_TAG, "Sensors initialized");
        do
        {
            read_sht3x_sensors();
            vTaskDelay(pdMS_TO_TICKS(sensor_history::sensor_interval / 50));

        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGE(OPERATIONS_TAG, "Hardware Task Failure:%s", ex.what());
        led_.set_color(255, 0, 0);
        // throw;
    }

    vTaskDelete(NULL);
}

void hardware::read_sht3x_sensors()
{
    const auto changed1 = read_sensor_if_time(sht3x_sensor1_, sht3x_sensor_last_read1_);
    const auto changed2 = read_sensor_if_time(sht3x_sensor2_, sht3x_sensor_last_read2_);

    if (changed1 || changed2)
    {
        // update average
        const auto value = get_sensor_value(sensor_id_index::humidity1) + get_sensor_value(sensor_id_index::humidity2);
        set_sensor_value(sensor_id_index::humidity, esp32::round_with_precision(value / 2.0, 1));
    }
}
