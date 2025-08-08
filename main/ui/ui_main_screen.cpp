#include "ui/ui_main_screen.h"
#include "logging/logging_tags.h"
#include "util/misc.h"
#include <esp_log.h>

void ui_main_screen::init()
{
    ui_screen::init();
    set_default_screen_color();

    refresh_timer_ = lv_timer_create(timer_callback<ui_main_screen, &ui_main_screen::update_slope>, 15000, this);

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

    auto humidity_image = lv_img_create(screen_);
    LV_IMG_DECLARE(humidity_png_img);
    lv_img_set_src(humidity_image, &humidity_png_img);
    lv_obj_align(humidity_image, LV_ALIGN_TOP_RIGHT, -5, 5);

    // Create images for slope
    up_slope_arrows[0] = create_images_for_slope(LV_ALIGN_BOTTOM_LEFT, 5, -5, 0);
    up_slope_arrows[1] = create_images_for_slope(LV_ALIGN_BOTTOM_RIGHT, -5, -5, 0);
    down_slope_arrows[0] = create_images_for_slope(LV_ALIGN_BOTTOM_LEFT, 5, -5, 1800);
    down_slope_arrows[1] = create_images_for_slope(LV_ALIGN_BOTTOM_RIGHT, -5, -5, 1800);

    update_slope(nullptr);

    lv_obj_add_event_cb(screen_, event_callback<ui_main_screen, &ui_main_screen::screen_callback>, LV_EVENT_ALL, this);
    ESP_LOGD(UI_TAG, "Main screen init done");
}

std::array<lv_obj_t *, 3> ui_main_screen::create_images_for_slope(lv_align_t align, int32_t x_ofs, int32_t y_ofs, int rotation)
{
    LV_IMG_DECLARE(upward_arrow_png_img);
    LV_IMG_DECLARE(upward_arrow_2_png_img);
    LV_IMG_DECLARE(upward_arrow_1_png_img);

    std::array<lv_obj_t *, 3> images;
    images[0] = lv_image_create(screen_);
    lv_img_set_src(images[0], &upward_arrow_png_img);
    lv_obj_align(images[0], LV_ALIGN_BOTTOM_LEFT, 5, -5);

    images[1] = lv_image_create(screen_);
    lv_img_set_src(images[1], &upward_arrow_2_png_img);
    lv_obj_align(images[1], LV_ALIGN_BOTTOM_LEFT, 5, -5);

    images[2] = lv_image_create(screen_);
    lv_img_set_src(images[2], &upward_arrow_1_png_img);
    lv_obj_align(images[2], LV_ALIGN_BOTTOM_LEFT, 5, -5);

    for (auto &image : images)
    {
        lv_obj_align(image, align, x_ofs, y_ofs);
        lv_img_set_angle(image, rotation);
    }

    return images;
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
            lv_label_set_text_fmt(humidity_label, "%lu", static_cast<unsigned long>(std::round(value)));
        }

        update_slope(nullptr);
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

void ui_main_screen::update_slope(lv_timer_t *)
{
    const auto minutes_to_consider = 1;
    const auto slope = ui_interface_instance_.get_sensor_slope_per_minute(minutes_to_consider, sensor_id_index::humidity);
    ESP_LOGI(UI_TAG, "Slope for humidity sensor: %f", slope);

    // hide all arrows first
    for (auto &slope_arrows : {up_slope_arrows, down_slope_arrows})
    {
        for (auto &arrow : slope_arrows)
        {
            for (auto &img : arrow)
            {
                lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    const auto slope_abs = std::abs(slope);
    if (slope_abs > 0)
    {
        uint8_t arrow_index = 0;

        if (slope_abs > 4)
        {
            arrow_index = 0; // 3 arrows
        }
        else if (slope_abs > 2)
        {
            arrow_index = 1; // 2 arrows
        }
        else
        {
            arrow_index = 2; // 1 arrow
        }

        auto &arrows = (slope > 0) ? up_slope_arrows : down_slope_arrows;
        for (auto &img_list : arrows)
        {
            lv_obj_clear_flag(img_list[arrow_index], LV_OBJ_FLAG_HIDDEN);
        }
    }
}
