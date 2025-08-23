#pragma once
#include "sdkconfig.h"

#include "hardware/sensors/sensor_id.h"
#include "util/singleton.h"
#include <array>
#include <tuple>
#include <esp_adc/adc_oneshot.h>
#include "util/noncopyable.h"

class photo_resistor_adc final : public esp32::noncopyable
{
  public:
    photo_resistor_adc(const std::array<sensor_id_index, 2>& ids) : sensor_ids(ids) {}
    void init(const std::array<adc_channel_t, 2>& channels);
    std::array<std::tuple<sensor_id_index, float>, 2> read();

    uint8_t get_initial_delay();

  private:
    std::array<adc_channel_t, 2> channels;
    adc_oneshot_unit_handle_t adc1_handle{};
     const std::array<sensor_id_index, 2> sensor_ids;
};
