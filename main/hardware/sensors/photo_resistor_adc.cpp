#include "hardware/sensors/photo_resistor_adc.h"

#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include "util/noncopyable.h"
#include <esp_log.h>

void photo_resistor_adc::init(const std::array<adc_channel_t, 2> &channels)
{
    this->channels = channels;
    adc_oneshot_unit_init_cfg_t init_config1 = {.unit_id = ADC_UNIT_1};

    CHECK_THROW_ESP(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {.atten = ADC_ATTEN_DB_0, .bitwidth = ADC_BITWIDTH_DEFAULT};

    for (const auto channel : channels)
    {
        CHECK_THROW_ESP(adc_oneshot_config_channel(adc1_handle, channel, &config));
        ESP_LOGI(HARDWARE_TAG, "Photo Resistor ADC initialized on channel %d", channel);
    }
}

std::array<std::tuple<sensor_id_index, float>, 2> photo_resistor_adc::read()
{
    std::array<std::tuple<sensor_id_index, float>, 2> results;
    for (int i = 0; i < channels.size(); i++)
    {
        int value{};
        const auto error = adc_oneshot_read(adc1_handle, channels[i], &value);

        if (error == ESP_OK)
        {
            ESP_LOGI(HARDWARE_TAG, "Read Photo Resistor ADC channel %d value:%d", channels[i], value);
            results[i] = std::tuple<sensor_id_index, float>{sensor_ids[i], static_cast<float>(value)};
        }
        else
        {
            ESP_LOGE(HARDWARE_TAG, "Failed to read Photo Resistor ADC channel %d error:%s", channels[i], esp_err_to_name(error));
            results[i] = std::tuple<sensor_id_index, float>{sensor_ids[i], NAN};
        }
    }
    return results;
}

uint8_t photo_resistor_adc::get_initial_delay()
{
    return 0; // no delay needed
}
