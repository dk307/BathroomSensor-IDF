#pragma once

#include "ui_screen.h"
#include <esp_log.h>

class ui_information_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void show_screen();

  private:
    lv_obj_t *system_table_{};
    lv_timer_t *refresh_timer_{};

    void screen_callback(lv_event_t *e);
    static lv_obj_t *create_table(lv_obj_t *tab);

    void load_information(lv_timer_t *);
};