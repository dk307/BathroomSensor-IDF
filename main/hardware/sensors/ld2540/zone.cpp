#include "zone.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include "util/misc.h"
#include <esp_log.h>

bool Zone::is_convex(const std::vector<Point> &polygon)
{
    if (polygon.size() < 3)
        return false;

    double last_cross_product = NAN;
    const auto size = polygon.size();
    for (int64_t i = 0; i < size + 1; i++)
    {
        int64_t dx_1 = polygon[(i + 1) % size].x - polygon[i % size].x;
        int64_t dy_1 = polygon[(i + 1) % size].y - polygon[i % size].y;
        int64_t dx_2 = polygon[(i + 2) % size].x - polygon[(i + 1) % size].x;
        int64_t dy_2 = polygon[(i + 2) % size].y - polygon[(i + 1) % size].y;
        double cross_product = dx_1 * dy_2 - dy_1 * dx_2;
        if (!std::isnan(last_cross_product) && ((cross_product > 0 && last_cross_product < 0) || (cross_product > 0 && last_cross_product < 0)))
            return false;
        last_cross_product = cross_product;
    }
    return true;
}

void Zone::update_from_targets()
{
    if (polygon_.size() < 3)
        return;

    uint8_t target_count = 0;
    for (auto i = 0; i < targets_.size(); i++)
    {
        target_count += contains_target(i);
    }

    target_count_sensor_ = target_count;
}

bool Zone::contains_target(size_t target_index)
{
    if (polygon_.size() < 3)
        return false;

    auto && target = targets_[target_index];

    // Check if the target is already beeing tracked
    bool is_tracked = tracked_targets_.contains(target_index);
    if (!target.is_present())
    {
        if (!is_tracked)
        {
            return false;
        }
        else
        {
            // Remove from tracking list after timeout (target did not leave via polygon boundary)
            if (esp32::millis() - tracked_targets_[target_index] > target_timeout_)
            {
                tracked_targets_.erase(target_index);
                return false;
            }
            else
            {
                // Report as contained as long as the target has not timed out
                return true;
            }
        }
    }

    const Point point(target.get_x(), target.get_y());

    const auto size = polygon_.size();
    bool is_inside = true;
    int16_t min_distance = INT16_MAX;
    double last_cross_product = NAN;
    // Check if the target is inside of the polygon or within the allowed margin, in case it is already tracked
    for (int64_t i = 0; i < size + 1; i++)
    {
        // Check if the target point is on the same side of all edges within the polygon
        int64_t dx_1 = polygon_[(i + 1) % size].x - polygon_[i % size].x;
        int64_t dy_1 = polygon_[(i + 1) % size].y - polygon_[i % size].y;
        int64_t dx_2 = point.x - polygon_[i % size].x;
        int64_t dy_2 = point.y - polygon_[i % size].y;
        double cross_product = dx_1 * dy_2 - dy_1 * dx_2;

        if (!std::isnan(last_cross_product) && ((cross_product > 0 && last_cross_product < 0) || (cross_product > 0 && last_cross_product < 0)))
        {
            is_inside = false;
            // Early stopping for un-tracked targets
            if (!is_tracked)
                return false;
        }
        last_cross_product = cross_product;

        // Determine the targets distance to the polygon if tracked
        if (is_tracked)
        {
            double dot_product = dx_1 * dx_2 + dy_1 * dy_2;
            double r = dot_product / std::pow(std::sqrt(dx_1 * dx_1 + dy_1 * dy_1), 2);

            double distance;
            if (r < 0)
            {
                distance = std::sqrt(dx_2 * dx_2 + dy_2 * dy_2);
            }
            else if (r > 1)
            {
                int64_t dx = polygon_[(i + 1) % size].x - point.x;
                int64_t dy = polygon_[(i + 1) % size].y - point.y;
                distance = std::sqrt(dx * dx + dy * dy);
            }
            else
            {
                double a = dx_2 * dx_2 + dy_2 * dy_2;
                double b = std::pow(std::sqrt(dx_1 * dx_1 + dy_1 * dy_1) * r, 2);
                distance = std::sqrt(a - b);
            }
            min_distance = std::min(min_distance, int16_t(distance));
        }
    }

    if (is_inside && target.is_present())
    {
        // Add and Update last seen time
        tracked_targets_[target_index] = esp32::millis();
    }
    else if (is_tracked && !is_inside)
    {
        // Check if the target is still within the margin of error
        if (min_distance > margin_)
        {
            // Remove from target from tracking list
            if (is_tracked)
                tracked_targets_.erase(target_index);
            return false;
        }
    }
    return true;
}
