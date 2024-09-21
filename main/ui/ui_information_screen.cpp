#include "ui/ui_information_screen.h"
#include "logging/logging_tags.h"
#include <esp_log.h>

void ui_information_screen::init()
{
    ui_screen::init();
    set_default_screen_color();
    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
    auto title = create_screen_title_static(4, "Information");

    lv_obj_add_event_cb(screen_, event_callback<ui_information_screen, &ui_information_screen::screen_callback>, LV_EVENT_ALL, this);
    system_table_ = create_table(screen_);
    lv_obj_align_to(system_table_, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    create_press_back_message();

    LV_IMG_DECLARE(info_png_img);
    set_background_image(&info_png_img);
}

void ui_information_screen::show_screen()
{
    ESP_LOGI(UI_TAG, "Showing information screen");
    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
}

void ui_information_screen::screen_callback(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_SCREEN_LOAD_START)
    {
        ESP_LOGD(UI_TAG, "setting screen shown");
        load_information(nullptr);
        refresh_timer_ = lv_timer_create(timer_callback<ui_information_screen, &ui_information_screen::load_information>, 1000, this);
    }
    else if (event_code == LV_EVENT_SCREEN_UNLOADED)
    {
        ESP_LOGD(UI_TAG, "setting screen hidden");
        if (refresh_timer_)
        {
            lv_timer_del(refresh_timer_);
            refresh_timer_ = nullptr;
        }
    }
}

lv_obj_t *ui_information_screen::create_table(lv_obj_t *tab)
{
    auto table = lv_table_create(tab);
    lv_obj_set_size(table, lv_pct(100), screen_height - 80);

    lv_obj_set_style_border_width(table, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(table, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(table, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(table, LV_OPA_0, LV_PART_ITEMS);
    return table;
}

void ui_information_screen::load_information(lv_timer_t *)
{
    const auto data = ui_interface_instance_.get_information_table(ui_interface::information_type::display);
    update_table(system_table_, data);
}
