#pragma once

#include "hardware/inbuild_led.h"
#include "hardware/sensors/ld2540/ld2450.h"
#include "hardware/sensors/sensor.h"
#include "hardware/sensors/sht3x_sensor_device.h"
#include "ui/ui_interface.h"
#include "util/psram_allocator.h"
#include "util/singleton.h"
#include <i2cdev.h>

class display;
class config;

class hardware final : public esp32::singleton<hardware>
{
  public:
    void begin();

    const sensor_value &get_sensor(sensor_id_index index) const
    {
        return sensors_[static_cast<uint8_t>(index)];
    }

    float get_sensor_value(sensor_id_index index) const;
    sensor_history::sensor_history_snapshot get_sensor_detail_info(sensor_id_index index);

    const sensor_history &get_sensor_history(sensor_id_index index) const
    {
        return (*sensors_history_)[static_cast<uint8_t>(index)];
    }

  private:
    hardware(config &config, display &display) : config_(config), display_(display), sensor_refresh_task_([this] { sensor_task_ftn(); })
    {
    }

    friend class esp32::singleton<hardware>;

    config &config_;
    display &display_;

    // same index as sensor_id_index
    std::array<sensor_value, total_sensors> sensors_;
    std::unique_ptr<std::array<sensor_history, total_sensors>, esp32::psram::deleter> sensors_history_ =
        esp32::psram::make_unique<std::array<sensor_history, total_sensors>>();

    esp32::task sensor_refresh_task_;

    // SHT31 - Sensor 1
    sht3x_sensor_device sht3x_sensor1_{sensor_id_index::humidity1};
    uint64_t sht3x_sensor_last_read1_ = 0;

    // SHT31 - Sensor 2
    sht3x_sensor_device sht3x_sensor2_{sensor_id_index::humidity2};
    uint64_t sht3x_sensor_last_read2_ = 0;

    inbuild_led led_;

    LD2450 ld2450_;

    void set_sensor_value(sensor_id_index index, float value);

    void read_sht3x_sensors();
    void sensor_task_ftn();

    template <class T> bool read_sensor_if_time(T &sensor, uint64_t &last_read);
};
