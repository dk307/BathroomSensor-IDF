#pragma once

#include <cmath>
#include <string>

/**
 * @brief Target component which provides information about a single target and updates derived sensor components.
 */
class Target
{
  public:
    void setup();

    /**
     * @brief Sets the name of this component
     */
    void set_name(const std::string &name)
    {
        name_ = name;
    }

    /**
     * @brief Sets the fast of detection flag, which determines how the unoccupied state is determined.
     */
    void set_fast_off_detection(bool flag)
    {
        fast_off_detection_ = flag;
    }

    /**
     * @brief Updates the value in this target object
     * @param x The x coordinate of the target
     * @param y The y coordinate of the target
     * @param speed The speed of the target
     * @param resolution The distance resolution of the measurement
     */
    void update_values(int16_t x, int16_t y, int16_t speed, int16_t resolution);

    /**
     * @brief Determines whether this target is currently detected.
     * @return true if the target is detected, false otherwise
     */
    bool is_present() const;

    /**
     * @brief Determines if this target is currently moving
     * @return true if the target is moving, false if it is stationary.
     */
    bool is_moving()const
    {
        return speed_ != 0;
    }

    /**
     * @brief Time since last last change in values.
     * @return timestamp in milliseconds since start
     */
    uint32_t get_last_change()const
    {
        return last_change_;
    }

    /**
     * @brief Rests all values in this target
     */
    void clear()
    {
        update_values(0, 0, 0, 0);
    }

    /**
     * @brief Gets the name of this target
     */
    const std::string &get_name()
    {
        return name_;
    }

    /**
     * Gets the x coordinate (horizontal position) of this targets
     *
     * @return horizontal position of the target (0 = center)
     */
    int16_t get_x() const
    {
        return x_;
    }

    /**
     * Gets the y coordinate (distance from the sensor) of this target
     * @return distance in centimeters
     */
    int16_t get_y() const
    {
        return y_;
    }

    /**
     * Gets the movement speed of this target
     * @return speed in m/s
     */
    int16_t get_speed() const
    {
        return speed_;
    }

    /**
     * Gets the distance resolution of this target
     * @return distance error
     */
    int16_t get_distance_resolution() const
    {
        return resolution_;
    }

    double get_angle() const
    {
        return std::atan2(y_, x_) * (180 / M_PI) - 90;
    }

    double get_distance() const
    {
        return std::sqrt(x_ * x_ + y_ * y_);
    }

  protected:
    /// @brief X (horizontal) coordinate of the target in relation to the sensor
    int16_t x_ = 0;

    /// @brief Y (distance) coordinate of the target in relation to the sensor
    int16_t y_ = 0;

    /// @brief speed of the target
    int16_t speed_ = 0;

    /// @brief distance resolution of the target
    int16_t resolution_ = 0;

    /// @brief  Name of this target
    std::string name_;

    /// @brief Determines whether the fast unoccupied detection method is applied
    bool fast_off_detection_ = false;

    /// @brief time stamp of the last debug message which was sent.
    uint32_t last_debug_message_ = 0;

    /// @brief time of the last value change
    uint32_t last_change_ = 0;
};
