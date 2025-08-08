#pragma once

#include "hardware/sensors/sensor.h"
#include "ui/ui_screen.h"

class ui_main_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void set_sensor_value(sensor_id_index index, float value);
    void show_screen();

  private:
    lv_obj_t *humidity_label{};
    lv_timer_t *refresh_timer_{};

    // images for slope
    // 0 is for left, 1 is for right
    // 0 is for 3 arrows, 2 is for 2 arrows, 0 is for 1 arrow
    std::array<std::array<lv_obj_t *, 3>, 2> up_slope_arrows{};
    std::array<std::array<lv_obj_t *, 3>, 2> down_slope_arrows{};

    void screen_callback(lv_event_t *e);
    std::array<lv_obj_t *, 3> create_images_for_slope(lv_align_t align, int32_t x_ofs, int32_t y_ofs, int rotation);

    void update_slope(lv_timer_t *);
};