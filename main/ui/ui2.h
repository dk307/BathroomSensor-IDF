#pragma once

#include "hardware/sensors/sensor.h"
#include "hardware/sensors/sensor_id.h"
#include "ui_boot_screen.h"
#include "ui_inter_screen_interface.h"
#include "ui_interface.h"
#include "ui_launcher_screen.h"
#include "ui_information_screen.h"
#include "ui_main_screen.h"
#include "ui_wifi_enroll_screen.h"
#include "util/noncopyable.h"
#include <lvgl.h>

class ui final : public ui_inter_screen_interface, esp32::noncopyable
{
  public:
    ui(config &config, ui_interface &ui_interface_) : config_(config), ui_interface_instance_(ui_interface_)
    {
    }
    void load_boot_screen();
    void init();
    void show_top_level_message(const std::string &message, uint32_t period = top_message_timer_period);
    void set_sensor_value(sensor_id_index id, float value);
    void set_main_screen();
    void wifi_changed();
    void update_button_timer(const std::optional<uint32_t> &data);

    // ui_inter_screen_interface
    bool is_night_theme_enabled() override;
    void show_home_screen() override;
    void show_setting_screen() override;
    void show_launcher_screen() override;
    void show_wifi_enroll_screen() override;

  private:
    config &config_;
    ui_interface &ui_interface_instance_;
    bool night_theme_{false};

    constexpr static uint32_t top_message_timer_period = 10000;

    // top sys layer
    lv_obj_t *no_wifi_image_{};
    lv_style_t no_wifi_image_style{};
    lv_anim_timeline_t *no_wifi_image_animation_timeline_{};
    lv_obj_t *button_timer_label_{};

    lv_obj_t *top_message_panel_{};
    lv_obj_t *top_message_label_{};
    lv_timer_t *top_message_timer_{};

    ui_boot_screen boot_screen_{config_, ui_interface_instance_, *this};
    ui_main_screen main_screen_{config_, ui_interface_instance_, *this};
    ui_information_screen settings_screen_{config_, ui_interface_instance_, *this};
    ui_launcher_screen launcher_screen_{config_, ui_interface_instance_, *this};
    ui_wifi_enroll_screen wifi_enroll_screen_{config_, ui_interface_instance_, *this};

    void init_no_wifi_image();
    void init_top_message();
    static void no_wifi_img_animation_cb(void *var, int32_t v);
    static void top_message_timer_cb(lv_timer_t *e);
    static lv_font_t *lv_font_from_sd_card(const char *path);
};
