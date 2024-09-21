#include "ui/ui_launcher_screen.h"
#include "logging/logging_tags.h"
#include <esp_log.h>

void ui_launcher_screen::init()
{
    LV_IMG_DECLARE(factory_reset_png_img);
    LV_IMG_DECLARE(wifi_png_img);
    LV_IMG_DECLARE(info_png_img);

    ui_screen::init();
    set_default_screen_color();

    int pad = 11;
    info_button_ = create_button(LV_ALIGN_TOP_LEFT, pad, pad, &info_png_img, "Info\n2 sec");
    wifi_button_ = create_button(LV_ALIGN_TOP_MID, 0, pad, &wifi_png_img, "Wifi\nEnroll\n5 sec");
    reset_button_ = create_button(LV_ALIGN_TOP_RIGHT, -pad, pad, &factory_reset_png_img, "Factory\nReset\n10 sec");

    instruction_label_ = lv_label_create(screen_);
    lv_obj_set_size(instruction_label_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(instruction_label_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_long_mode(instruction_label_, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(instruction_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(instruction_label_, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_static(instruction_label_, "Press and hold to select.\nDouble click to go to main screen");    
}

void ui_launcher_screen::show_screen()
{
    update_button_timer(std::nullopt);
    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
}

void ui_launcher_screen::update_button_timer(const std::optional<uint32_t> &data)
{
    if (data.has_value())
    {
        lv_obj_add_flag(instruction_label_, LV_OBJ_FLAG_HIDDEN);
        if (data.value() >= factory_reset_long_press_time)
        {
            select_button(reset_button_);
        }
        else if (data.value() >= wifi_enroll_long_press_time)
        {
            select_button(wifi_button_);
        }
        else if (data.value() >= info_long_press_time)
        {
            select_button(info_button_);
        }
        else
        {
            select_button(nullptr);
        }
    }
    else
    {
        lv_obj_clear_flag(instruction_label_, LV_OBJ_FLAG_HIDDEN);
        select_button(nullptr);
    }
}

lv_obj_t *ui_launcher_screen::create_button(lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const lv_img_dsc_t *src, const char *label_str)
{
    auto btn = lv_img_create(screen_);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_img_set_src(btn, src);

    auto label = lv_label_create(screen_);
    lv_label_set_text_static(label, label_str);
    lv_obj_align_to(label, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    return btn;
}

void ui_launcher_screen::select_button(lv_obj_t *selected)
{
    lv_obj_t *buttons[] = {info_button_, wifi_button_, reset_button_};

    for (auto button : buttons)
    {
        if (button == selected)
        {
            lv_img_set_zoom(button, 312);
            lv_obj_set_style_img_opa(button, LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_img_set_zoom(button, 256);
            lv_obj_set_style_img_opa(button, selected ? LV_OPA_50 : LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}
