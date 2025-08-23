#include "ui2.h"
#include "logging/logging_tags.h"
#include "ui_interface.h"
#include <esp_log.h>
#include <memory>
#include <tuple>

lv_img_dsc_t logo_img;

#if LV_USE_LOG
void lv_logger(lv_log_level_t level, const char *buf)
{
    switch (level)
    {
    case LV_LOG_LEVEL_INFO:
        ESP_LOGI(UI_TAG, "%s", buf);
        break;
    case LV_LOG_LEVEL_WARN:
        ESP_LOGW(UI_TAG, "%s", buf);
        break;
    case LV_LOG_LEVEL_ERROR:
        ESP_LOGE(UI_TAG, "%s", buf);
        break;
    default:
        ESP_LOGI(UI_TAG, "Unknown log level %d: %s", level, buf);
        break;
    }
}
#endif

void ui::no_wifi_img_animation_cb(void *var, int32_t v)
{
    auto pThis = reinterpret_cast<ui *>(var);
    const auto op = v > 256 ? 512 - v : v;
    lv_style_set_img_opa(&pThis->no_wifi_image_style, op);
    lv_obj_refresh_style(pThis->no_wifi_image_, LV_PART_ANY, LV_STYLE_PROP_ANY);
}

void ui::load_boot_screen()
{
#if LV_USE_LOG
    lv_log_register_print_cb(&lv_logger);
#endif
    lv_disp_t *dispp = lv_disp_get_default();

    // most of ui uses this theme except main & detail sensor screen
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_LIME), true, LV_FONT_DEFAULT);

    lv_disp_set_theme(dispp, theme);

    boot_screen_.init();
    boot_screen_.show_screen();

    ESP_LOGI(UI_TAG, "Loaded boot screen");
}

void ui::init()
{
    // dont't cache boot screen
    lv_image_cache_drop(NULL);

    init_top_message();
    init_no_wifi_image();

    ESP_LOGI(UI_TAG, "Initializing UI screens");
    main_screen_.init();
    launcher_screen_.init();
    settings_screen_.init();
    wifi_enroll_screen_.init();

    button_timer_label_ = lv_label_create(lv_layer_sys());
    lv_obj_set_size(button_timer_label_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(button_timer_label_, LV_ALIGN_BOTTOM_MID, -10, 0);
    lv_obj_set_style_text_align(button_timer_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(button_timer_label_, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(button_timer_label_, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(button_timer_label_, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(UI_TAG, "UI screens initialized");
}

void ui::init_no_wifi_image()
{
    LV_IMG_DECLARE(nowifi_png_img);

    no_wifi_image_ = lv_img_create(lv_layer_sys());
    lv_img_set_src(no_wifi_image_, &nowifi_png_img);
    lv_obj_align(no_wifi_image_, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_flag(no_wifi_image_, LV_OBJ_FLAG_HIDDEN);

    lv_style_init(&no_wifi_image_style);
    lv_style_set_img_opa(&no_wifi_image_style, 100);
    lv_obj_add_style(no_wifi_image_, &no_wifi_image_style, 0);

    lv_anim_t no_wifi_image_animation;
    lv_anim_init(&no_wifi_image_animation);
    lv_anim_set_var(&no_wifi_image_animation, this);
    lv_anim_set_values(&no_wifi_image_animation, 0, 512);
    lv_anim_set_time(&no_wifi_image_animation, 2000);
    lv_anim_set_exec_cb(&no_wifi_image_animation, no_wifi_img_animation_cb);
    lv_anim_set_repeat_count(&no_wifi_image_animation, LV_ANIM_REPEAT_INFINITE);

    no_wifi_image_animation_timeline_ = lv_anim_timeline_create();
    lv_anim_timeline_add(no_wifi_image_animation_timeline_, 0, &no_wifi_image_animation);
}

void ui::top_message_timer_cb(lv_timer_t *e)
{
    auto p_this = reinterpret_cast<ui *>(lv_timer_get_user_data(e));
    lv_obj_add_flag(p_this->bottom_message_panel_, LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(p_this->top_message_timer_);
}

void ui::init_top_message()
{
    bottom_message_panel_ = lv_obj_create(lv_layer_sys());
    lv_obj_set_size(bottom_message_panel_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(bottom_message_panel_, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_border_width(bottom_message_panel_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bottom_message_panel_, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bottom_message_panel_, LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bottom_message_panel_, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(bottom_message_panel_, lv_color_hex(0xF5F5F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(bottom_message_panel_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(bottom_message_panel_, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(bottom_message_panel_, LV_OBJ_FLAG_GESTURE_BUBBLE);

    top_message_label_ = lv_label_create(bottom_message_panel_);
    lv_obj_set_size(bottom_message_panel_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(top_message_label_, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_long_mode(top_message_label_, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(top_message_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(top_message_label_, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(top_message_label_, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);

    top_message_timer_ = lv_timer_create(top_message_timer_cb, top_message_timer_period, this);
    lv_timer_pause(top_message_timer_);
}

void ui::set_sensor_value(sensor_id_index index, float value)
{
    if (main_screen_.is_active())
    {
        main_screen_.set_sensor_value(index, value);
    }
}

void ui::show_top_level_message(const std::string &message, uint32_t period)
{
    ESP_LOGI(UI_TAG, "Showing top level message:%s", message.c_str());
    lv_label_set_text(top_message_label_, message.c_str());
    lv_obj_clear_flag(bottom_message_panel_, LV_OBJ_FLAG_HIDDEN);
    lv_timer_reset(top_message_timer_);
    lv_timer_resume(top_message_timer_);
}

void ui::set_main_screen()
{
    main_screen_.show_screen();
    wifi_changed();
}

void ui::wifi_changed()
{
    if (!boot_screen_.is_active())
    {
        const auto wifi_status = ui_interface_instance_.get_wifi_status();
        if (wifi_status.connected)
        {
            ESP_LOGI(UI_TAG, "Hiding No wifi icon");
            lv_obj_add_flag(no_wifi_image_, LV_OBJ_FLAG_HIDDEN);
            lv_anim_timeline_pause(no_wifi_image_animation_timeline_);
        }
        else
        {
            ESP_LOGI(UI_TAG, "Showing No wifi icon");
            lv_obj_clear_flag(no_wifi_image_, LV_OBJ_FLAG_HIDDEN);
            lv_anim_timeline_start(no_wifi_image_animation_timeline_);
        }

        show_top_level_message(wifi_status.status, 4000);

        if (wifi_enroll_screen_.is_active() && wifi_status.connected)
        {
            show_home_screen();
        }
    }
}

void ui::update_button_timer(const std::optional<uint32_t> &data)
{
    if (data.has_value())
    {
        const auto value = data.value() / 1000;
        lv_label_set_text_fmt(button_timer_label_, "%lu second%s", value, value <= 1 ? "" : "s");
        lv_obj_clear_flag(button_timer_label_, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(button_timer_label_, LV_OBJ_FLAG_HIDDEN);
    }

    if (launcher_screen_.is_active())
    {
        launcher_screen_.update_button_timer(data);
    }
    else if (wifi_enroll_screen_.is_active())
    {
        wifi_enroll_screen_.update_button_timer(data);
    }
    else if (settings_screen_.is_active())
    {
        settings_screen_.update_button_timer(data);
    }
}

void ui::show_home_screen()
{
    main_screen_.show_screen();
}

void ui::show_setting_screen()
{
    if (!settings_screen_.is_active())
    {
        settings_screen_.show_screen();
    }
}

void ui::show_launcher_screen()
{
    if (!launcher_screen_.is_active())
    {
        lv_obj_add_flag(bottom_message_panel_, LV_OBJ_FLAG_HIDDEN);
        launcher_screen_.show_screen();
    }
}

void ui::show_wifi_enroll_screen()
{
    if (!wifi_enroll_screen_.is_active())
    {
        lv_obj_add_flag(bottom_message_panel_, LV_OBJ_FLAG_HIDDEN);
        wifi_enroll_screen_.show_screen();
    }
}

bool ui::is_night_theme_enabled()
{
    return night_theme_;
}


void ui::set_day_or_night_theme(bool night_mode)
{
    if (night_theme_ != night_mode)
    {
        ESP_LOGI(UI_TAG, "Setting theme :%d", night_mode);
        night_theme_ = night_mode;
        main_screen_.theme_changed();
        settings_screen_.theme_changed();
        launcher_screen_.theme_changed();
        wifi_enroll_screen_.theme_changed();
    }
}