#include "ui/ui_wifi_enroll_screen.h"

void ui_wifi_enroll_screen::init()
{
    ui_screen::init();
    set_default_screen_color();

    create_screen_title_static(4, "Wifi Enroll");

    lv_obj_add_event_cb(screen_, event_callback<ui_wifi_enroll_screen, &ui_wifi_enroll_screen::screen_callback>, LV_EVENT_ALL, this);

    // message label
    auto *label = create_a_label(screen_, &lv_font_montserrat_20, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_static(label, "Use ESPTouch app to set Wifi\ncredentials for the device");

    create_press_back_message();

    LV_IMG_DECLARE(wifi_png_img);
    set_background_image(&wifi_png_img );
}

void ui_wifi_enroll_screen::show_screen()
{
    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
}

void ui_wifi_enroll_screen::screen_callback(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_SCREEN_LOAD_START)
    {
        ui_interface_instance_.start_wifi_enrollment();
    }
    else if (event_code == LV_EVENT_SCREEN_UNLOADED)
    {
        ui_interface_instance_.stop_wifi_enrollment();
    }
}