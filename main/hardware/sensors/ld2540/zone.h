#pragma once
#include "target.h"
#include <functional>
#include <map>
#include <span>
#include <vector>

/**
 * @brief Simple point class which describes Cartesian location.
 */
struct Point
{
    Point() = default;
    Point(int64_t x, int64_t y) : x(x), y(y)
    {
    }

    int64_t x{};
    int64_t y{};
};

struct zone_data
{
    std::string name;
    std::vector<Point> polygons;
    float margin_meter;
    int32_t target_timeout_ms;
};

/**
 * @brief Zones describe a phyiscal area, in which target are tracked. The area is given by a convex polygon.
 */
class Zone
{
  public:
    Zone(const zone_data &data, const std::span<const Target> &targets)
        : name_(data.name), polygon_(data.polygons), margin_(data.margin_meter * 1000), target_timeout_(data.target_timeout_ms), targets_(targets)
    {
    }

    void update_from_targets();

    /**
     * Gets the occupancy status of this Zone.
     * @return true, if at least one target is present in this zone.
     */
    bool is_occupied()
    {
        return tracked_targets_.size() > 0;
    }

    /**
     * @brief Gets the number of targets currently occupying this zone.
     * @return number of targets
     */
    uint8_t get_target_count()
    {
        return tracked_targets_.size();
    }

    static bool is_convex(const std::vector<Point> &polygon);

  private:
    /**
     * @brief checks if a Target is contained within the zone
     * @return true if the target is currently tracked inside this zone.
     */
    bool contains_target(size_t target);

    /// @brief Name of this zone
    const std::string name_;

    /// @brief List of points which make up a convex polygon
    const std::vector<Point> polygon_{};

    /// @brief Margin around the polygon, which still in mm
    const uint16_t margin_;

    /// @brief timeout after which a target within the is considered absent
    const int target_timeout_;

    /// @brief Map of targets which are currently tracked inside of this polygon with their last seen timestamp
    std::map<size_t, uint32_t> tracked_targets_{};

    const std::span<const Target> &targets_;

    double target_count_sensor_{NAN};
};
