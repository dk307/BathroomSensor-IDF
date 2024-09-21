#pragma once

#include "ui_inter_screen_interface.h"
#include "ui_interface.h"
#include "util/noncopyable.h"
#include <lvgl.h>

LV_FONT_DECLARE(big_panel_font);

class ui_screen : esp32::noncopyable
{
  public:
    ui_screen(config &config, ui_interface &ui_interface_instance_, ui_inter_screen_interface &ui_inter_screen_interface);

    virtual void init();
    bool is_active() const;

    virtual void update_button_timer(const std::optional<uint32_t> &data);

  protected:
    constexpr static int screen_width = 240;
    constexpr static int screen_height = 320;

    config &config_;
    ui_interface &ui_interface_instance_;
    ui_inter_screen_interface &inter_screen_interface_;
    lv_obj_t *screen_{};
    lv_obj_t *back_text_label_{};

    template <class T, void (T::*ftn)(lv_event_t *)> static void event_callback(lv_event_t *e)
    {
        auto p_this = reinterpret_cast<T *>(lv_event_get_user_data(e));
        (p_this->*ftn)(e);
    }

    template <class T, void (T::*ftn)(lv_timer_t *)> static void timer_callback(lv_timer_t *e)
    {
        auto p_this = reinterpret_cast<T *>(e->user_data);
        (p_this->*ftn)(e);
    }

    void create_press_back_message();
    void set_default_screen_color();
    void set_background_image(const void * src);
    lv_obj_t *create_screen_title_static(lv_coord_t y_ofs, const char *title);

    static lv_obj_t *create_a_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
    static void update_table(lv_obj_t *table, const ui_interface::information_table_type &data);
    static void set_padding_zero(lv_obj_t *obj);

  private:
    static void event_callback_ftn(lv_event_t *e);
};
