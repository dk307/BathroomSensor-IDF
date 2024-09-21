#pragma once

#include "sdkconfig.h"
#include <cstddef>
#include <stdint.h>

enum class sensor_id_index : uint8_t
{
    humidity1,
    first = humidity1,
    humidity2,
    humidity, // average
    last = humidity,
};

constexpr auto total_sensors = static_cast<size_t>(sensor_id_index::last) + 1;

enum class sensor_level : uint8_t
{
    no_level = 0,
    level_1 = 1,
    level_2 = 2,
    level_3 = 3,
    level_4 = 4,
    level_5 = 5,
    level_6 = 6,
};
