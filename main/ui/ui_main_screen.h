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

    void screen_callback(lv_event_t *e);
};