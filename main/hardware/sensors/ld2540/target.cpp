#include "target.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include "util/misc.h"
#include <esp_log.h>

#define FAST_OFF_THRESHOLD 100

void Target::update_values(int16_t x, int16_t y, int16_t speed, int16_t resolution)
{
    if (fast_off_detection_ && resolution_ != 0 && (x != x_ || y != y_ || speed != speed_ || resolution != resolution_))
    {
        last_change_ = esp32::millis();
    }
    x_ = x;
    y_ = y;
    speed_ = speed;
    resolution_ = resolution;
}

bool Target::is_present() const
{
    return resolution_ != 0 && (!fast_off_detection_ || esp32::millis() - last_change_ <= FAST_OFF_THRESHOLD);
}
