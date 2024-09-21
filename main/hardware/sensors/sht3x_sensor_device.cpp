

#include "hardware/sensors/sht3x_sensor_device.h"

#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include "util/noncopyable.h"
#include <esp_log.h>
#include <i2cdev.h>

void sht3x_sensor_device::init(i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    // sht3x
    CHECK_THROW_ESP(sht3x_init_desc(&sht3x_sensor_, SHT3X_I2C_ADDR_GND, port, sda_gpio, scl_gpio));
    CHECK_THROW_ESP(sht3x_init(&sht3x_sensor_));
}

std::array<std::tuple<sensor_id_index, float>, 1> sht3x_sensor_device::read()
{
    float temperatureC = NAN;
    float humidity = NAN;
    auto err = sht3x_measure(&sht3x_sensor_, &temperatureC, &humidity);
    if (err == ESP_OK)
    {
        ESP_LOGI(SENSOR_SHT31_TAG, "Read SHT31 sensor values:%g C  %g %%", temperatureC, humidity);
    }
    else
    {
        ESP_LOGE(SENSOR_SHT31_TAG, "Failed to read from SHT3x sensor with error:%s", esp_err_to_name(err));
    }

    return {std::tuple<sensor_id_index, float>{sensor_id, esp32::round_with_precision(humidity, 2)}};
}

uint8_t sht3x_sensor_device::get_initial_delay()
{
    return sht3x_get_measurement_duration(SHT3X_HIGH);
}
