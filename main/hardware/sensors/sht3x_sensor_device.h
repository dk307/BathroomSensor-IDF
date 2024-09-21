#pragma once
#include "sdkconfig.h"

#include "hardware/sensors/sensor_id.h"
#include "util/singleton.h"
#include <array>
#include <i2cdev.h>
#include <sht3x.h>
#include <tuple>
#include "util/noncopyable.h"

class sht3x_sensor_device final : public esp32::noncopyable
{
  public:
    sht3x_sensor_device(sensor_id_index id) : sensor_id(id) {}
    void init(i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
    std::array<std::tuple<sensor_id_index, float>, 1> read();

    uint8_t get_initial_delay();

  private:
    sht3x_t sht3x_sensor_{};
    const sensor_id_index sensor_id;
};
