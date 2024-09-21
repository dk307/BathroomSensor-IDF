

#include "ui/ui_screen.h"
#include "logging/logging_tags.h"
#include "ui/ui_inter_screen_interface.h"
#include "ui/ui_interface.h"
#include "util/noncopyable.h"
#include <esp_log.h>
#include <lvgl.h>

ui_screen::ui_screen(config &config, ui_interface &ui_interface_instance_, ui_inter_screen_interface &ui_inter_screen_interface)
    : config_(config), ui_interface_instance_(ui_interface_instance_), inter_screen_interface_(ui_inter_screen_interface)
{
}

void ui_screen::init()
{
    screen_ = lv_obj_create(NULL);
}

bool ui_screen::is_active() const
{
    return lv_scr_act() == screen_;
}

void ui_screen::set_padding_zero(lv_obj_t *obj)
{
    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_screen::set_default_screen_color()
{
    lv_obj_set_style_bg_grad_dir(screen_, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x080402), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(screen_, lv_color_hex(0x060606), LV_PART_MAIN | LV_STATE_DEFAULT);
}

lv_obj_t *ui_screen::create_a_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    auto *label = lv_label_create(parent);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(label, align, x_ofs, y_ofs);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);

    return label;
}

void ui_screen::update_table(lv_obj_t *table, const ui_interface::information_table_type &data)
{
    lv_table_set_col_cnt(table, 2);
    lv_table_set_row_cnt(table, data.size());

    lv_table_set_col_width(table, 0, 140);
    lv_table_set_col_width(table, 1, screen_width - 5);

    for (auto i = 0; i < data.size(); i++)
    {
        lv_table_set_cell_value(table, i, 0, std::get<0>(data[i]).data());
        lv_table_set_cell_value(table, i, 1, std::get<1>(data[i]).c_str());
    }
}

lv_obj_t *ui_screen::create_screen_title_static(lv_coord_t y_ofs, const char *title)
{
    auto label = create_a_label(screen_, &lv_font_montserrat_32, LV_ALIGN_TOP_MID, 0, y_ofs);
    lv_label_set_text_static(label, title);
    return label;
}

void ui_screen::event_callback_ftn(lv_event_t *e)
{
    auto p_ftn = reinterpret_cast<std::function<void(lv_event_t * e)> *>(lv_event_get_user_data(e));
    (*p_ftn)(e);
}

void ui_screen::create_press_back_message()
{
    back_text_label_ = create_a_label(screen_, &lv_font_montserrat_18, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_text_static(back_text_label_, "Press button to go back");
}

void ui_screen::update_button_timer(const std::optional<uint32_t> &data)
{
    if (back_text_label_)
    {
        if (data.has_value())
        {
            lv_obj_add_flag(back_text_label_, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(back_text_label_, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_screen::set_background_image(const void *src)
{
    auto bg_image = lv_img_create(screen_);
    lv_img_set_src(bg_image, src);
    lv_obj_set_size(bg_image, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(bg_image, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_img_opa(bg_image, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_img_set_zoom(bg_image, 512);

    auto image = lv_img_create(screen_);
    lv_img_set_src(image, src);
    lv_obj_align(image, LV_ALIGN_TOP_RIGHT, 25, -25);
    lv_img_set_zoom(image, 128);
}
