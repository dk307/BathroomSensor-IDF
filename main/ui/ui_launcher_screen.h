#pragma once

#include "ui/ui_screen.h"

class ui_launcher_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void show_screen();

    void update_button_timer(const std::optional<uint32_t> &data) override;

    static constexpr int info_long_press_time = 2000;
    static constexpr int wifi_enroll_long_press_time = 5000;
    static constexpr int factory_reset_long_press_time = 10000;

  private:
    lv_obj_t *info_button_{};
    lv_obj_t *wifi_button_{};
    lv_obj_t *reset_button_{};
    lv_obj_t *instruction_label_{};

    lv_obj_t *create_button(lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const lv_img_dsc_t *src, const char *label_str);
    void select_button(lv_obj_t *selected);
};