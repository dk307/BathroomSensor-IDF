#pragma once

#include "ui/ui_screen.h"

class ui_wifi_enroll_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void show_screen();

  private:
    void screen_callback(lv_event_t *e);
};