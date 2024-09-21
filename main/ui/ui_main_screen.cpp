#include "ui/ui_main_screen.h"
#include "logging/logging_tags.h"
#include "util/misc.h"
#include <esp_log.h>

void ui_main_screen::init()
{
    ui_screen::init();
    set_default_screen_color();

    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
    auto label = create_a_label(screen_, &lv_font_montserrat_32, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_static(label, "Humidity");

    humidity_label = lv_label_create(screen_);
    lv_obj_set_size(humidity_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(humidity_label, LV_ALIGN_CENTER, 0, 15);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(humidity_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(humidity_label, &big_panel_font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_static(humidity_label, "-");

    auto image = lv_img_create(screen_);
    LV_IMG_DECLARE(humidity_png_img);
    lv_img_set_src(image, &humidity_png_img);
    lv_obj_align(image, LV_ALIGN_TOP_RIGHT, -5, 5);

    lv_obj_add_event_cb(screen_, event_callback<ui_main_screen, &ui_main_screen::screen_callback>, LV_EVENT_ALL, this);
    ESP_LOGD(UI_TAG, "Main screen init done");
}

void ui_main_screen::set_sensor_value(sensor_id_index index, float value)
{
    if (index == sensor_id_index::humidity)
    {
        ESP_LOGI(UI_TAG, "Updating sensor %.*s to %g in main screen", get_sensor_name(index).size(), get_sensor_name(index).data(), value);
        if (std::isnan(value)) 
        {
            lv_label_set_text_static(humidity_label, "-");
        } 
        else
        {
            lv_label_set_text_fmt(humidity_label, "%g", value);
        }
    }
}

void ui_main_screen::show_screen()
{
    ESP_LOGI(UI_TAG, "Showing main screen");
    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
}

void ui_main_screen::screen_callback(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if (event_code == LV_EVENT_LONG_PRESSED)
    {
        ESP_LOGI(UI_TAG, "Long press detected");
        inter_screen_interface_.show_launcher_screen();
    }
    else if (event_code == LV_EVENT_SCREEN_LOAD_START)
    {
        set_sensor_value(sensor_id_index::humidity, ui_interface_instance_.get_sensor_value(sensor_id_index::humidity));
    }
}
