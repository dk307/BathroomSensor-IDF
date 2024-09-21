#include "display.h"
#include "config/config_manager.h"
#include "hardware/hardware.h"
#include "logging/logging_tags.h"
#include "operations/operations.h"
#include "ui/ui2.h"
#include "ui/ui_launcher_screen.h"
#include "util/cores.h"
#include "util/default_event.h"
#include "util/exceptions.h"
#include "wifi/wifi_manager.h"
#include <esp_log.h>
#include <lvgl.h>

const int LV_TICK_PERIOD_MS = 1;

/* Display flushing */
void display::display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    auto display_device_ = reinterpret_cast<LGFX *>(disp->user_data);
    if (display_device_->getStartCount() == 0)
    {
        display_device_->endWrite();
    }

    display_device_->pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t *)&color_p->full);

    lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

void display::begin()
{
    ESP_LOGI(DISPLAY_TAG, "Setting up display");

    instance_app_common_event_.subscribe();

    lv_init();

    if (!display_device_.init())
    {
        CHECK_THROW_ESP2(ESP_FAIL, "Failed to init display");
    }

    display_device_.setRotation(1);
    display_device_.initDMA();
    display_device_.startWrite();

    const auto screenWidth = display_device_.width();
    const auto screenHeight = display_device_.height();

    ESP_LOGI(DISPLAY_TAG, "Display initialized width:%ld height:%ld", screenWidth, screenHeight);

    ESP_LOGI(DISPLAY_TAG, "LV initialized");
    const int buffer_size = 80;

    const auto display_buffer_size = screenWidth * buffer_size * sizeof(lv_color_t);
    ESP_LOGI(DISPLAY_TAG, "Display buffer size:%ld", display_buffer_size);
    disp_draw_buf_ = (lv_color_t *)heap_caps_malloc(display_buffer_size, MALLOC_CAP_DMA);
    disp_draw_buf2_ = (lv_color_t *)heap_caps_malloc(display_buffer_size, MALLOC_CAP_DMA);

    if (!disp_draw_buf_ || !disp_draw_buf2_)
    {
        CHECK_THROW_ESP2(ESP_ERR_NO_MEM, "Failed to allocate lvgl display buffer");
    }

    lv_disp_draw_buf_init(&draw_buf_, disp_draw_buf_, disp_draw_buf2_, screenWidth * buffer_size);

    ESP_LOGD(DISPLAY_TAG, "LVGL display buffer initialized");

    /*** LVGL : Setup & Initialize the display device driver ***/
    lv_disp_drv_init(&disp_drv_);
    disp_drv_.hor_res = display_device_.width();
    disp_drv_.ver_res = display_device_.height();
    disp_drv_.flush_cb = display_flush;
    disp_drv_.draw_buf = &draw_buf_;
    disp_drv_.user_data = &display_device_;
    lv_display_ = lv_disp_drv_register(&disp_drv_);

    ESP_LOGD(DISPLAY_TAG, "LVGL display initialized");

    CHECK_THROW_ESP(lvgl_task_.spawn_pinned("lv_gui", 1024 * 6, esp32::task::default_priority, esp32::display_core));

    ESP_LOGI(DISPLAY_TAG, "Display setup done");

    button_config_t gpio_btn_cfg{};

    gpio_btn_cfg.type = BUTTON_TYPE_GPIO;
    gpio_btn_cfg.long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS;
    gpio_btn_cfg.short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS;
    gpio_btn_cfg.gpio_button_config.gpio_num = 46;
    gpio_btn_cfg.gpio_button_config.active_level = 0;
    gpio_btn_cfg.gpio_button_config.enable_power_save = true;
    button_ = iot_button_create(&gpio_btn_cfg);

    CHECK_THROW_ESP(iot_button_register_cb(button_, BUTTON_SINGLE_CLICK, button_event_callback<&display::button_click>, this));
    CHECK_THROW_ESP(iot_button_register_cb(button_, BUTTON_DOUBLE_CLICK, button_event_callback<&display::button_double_click>, this));
    CHECK_THROW_ESP(iot_button_register_cb(button_, BUTTON_LONG_PRESS_UP, button_event_callback<&display::button_long_press_up>, this));
    CHECK_THROW_ESP(iot_button_register_cb(button_, BUTTON_LONG_PRESS_HOLD, button_event_callback<&display::button_long_press_hold>, this));
}

void display::button_click()
{
    ESP_LOGI(DISPLAY_TAG, "Button clicked");
    xTaskNotify(lvgl_task_.handle(), set_launcher_screen_changed_bit, eSetBits);
}

void display::button_double_click()
{
    ESP_LOGI(DISPLAY_TAG, "Button double clicked");
    xTaskNotify(lvgl_task_.handle(), set_main_screen_changed_bit, eSetBits);
}

void display::button_long_press_up()
{
    const auto time = iot_button_get_ticks_time(button_);

    if (time >= ui_launcher_screen::factory_reset_long_press_time)
    {
        ESP_LOGI(DISPLAY_TAG, "Button long press up after %u ms for Factory Reset", time);
        // Factory Reset
        operations::instance.factory_reset();
    }
    else if (time >= ui_launcher_screen::wifi_enroll_long_press_time)
    {
        ESP_LOGI(DISPLAY_TAG, "Button long press up after %u ms for Wifi Enroll", time);
        // Wifi Enroll
        xTaskNotify(lvgl_task_.handle(), set_wifi_enroll_screen_changed_bit, eSetBits);
    }
    else if (time >= ui_launcher_screen::info_long_press_time)
    {
        ESP_LOGI(DISPLAY_TAG, "Button long press up after %u ms for Info", time);
        // Info Screen
        xTaskNotify(lvgl_task_.handle(), set_info_screen_changed_bit, eSetBits);
    }
    else
    {
        ESP_LOGI(DISPLAY_TAG, "Button long press up after %u ms with no action", time);
    }

    xTaskNotify(lvgl_task_.handle(), set_update_button_timer_changed_bit, eSetBits);
}

void display::button_long_press_hold()
{
    const auto time = iot_button_get_ticks_time(button_);
    ESP_LOGD(DISPLAY_TAG, "Button long press %u ms", time);
    xTaskNotify(lvgl_task_.handle(), set_update_button_timer_changed_bit, eSetBits);
}

void display::gui_task()
{
    ESP_LOGI(DISPLAY_TAG, "Start to run LVGL Task on core:%d", xPortGetCoreID());

    try
    {
        ui_instance_.load_boot_screen();
        lv_task_handler();
        ui_instance_.init();

        do
        {
            lv_task_handler();

            uint32_t notification_value = 0;
            const auto result = xTaskNotifyWait(pdFALSE,             /* Don't clear bits on entry. */
                                                ULONG_MAX,           /* Clear all bits on exit. */
                                                &notification_value, /* Stores the notified value. */
                                                pdMS_TO_TICKS(10));

            if (result == pdPASS)
            {
                if (notification_value & task_notify_wifi_changed_bit)
                {
                    ui_instance_.wifi_changed();
                }

                // screens change bits
                if (notification_value & set_main_screen_changed_bit)
                {
                    ui_instance_.set_main_screen();
                }
                else if (notification_value & set_launcher_screen_changed_bit)
                {
                    ui_instance_.show_launcher_screen();
                }
                else if (notification_value & set_wifi_enroll_screen_changed_bit)
                {
                    ui_instance_.show_wifi_enroll_screen();
                }
                else if (notification_value & set_info_screen_changed_bit)
                {
                    ui_instance_.show_setting_screen();
                }

                if (notification_value & task_notify_restarting_bit)
                {
                    ui_instance_.show_top_level_message("Restarting", 600000);
                }

                if (notification_value & idenitfy_device_bit)
                {
                    ui_instance_.show_top_level_message("Device Identify", 10000);
                }

                if (notification_value & set_update_button_timer_changed_bit)
                {
                    const auto state = iot_button_get_key_level(button_);
                    if (state)
                    {
                        const auto time = iot_button_get_ticks_time(button_);
                        ui_instance_.update_button_timer(time);
                    }
                    else
                    {
                        ui_instance_.update_button_timer(std::nullopt);
                    }
                }

                for (auto i = 1; i <= total_sensors; i++)
                {
                    if ((notification_value & BIT(i)) || (notification_value & config_changed_bit))
                    {
                        const auto id = static_cast<sensor_id_index>(i - 1);
                        const auto &sensor = ui_interface_.get_sensor(id);
                        const auto value = sensor.get_value();
                        ui_instance_.set_sensor_value(id, value);
                    }
                }
            }
        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGI(OPERATIONS_TAG, "UI Task Failure:%s", ex.what());
        throw;
    }

    vTaskDelete(NULL);
}

void display::app_event_handler(esp_event_base_t, int32_t event, void *data)
{
    switch (event)
    {
    case APP_INIT_DONE:
        xTaskNotify(lvgl_task_.handle(), set_main_screen_changed_bit, eSetBits);
        break;
    case SENSOR_VALUE_CHANGE: {
        const auto id = (*reinterpret_cast<sensor_id_index *>(data));
        xTaskNotify(lvgl_task_.handle(), BIT(static_cast<uint8_t>(id) + 1), eSetBits);
    }
    break;
    case WIFI_STATUS_CHANGED:
        xTaskNotify(lvgl_task_.handle(), task_notify_wifi_changed_bit, eSetBits);
        break;
    case APP_EVENT_REBOOT:
        xTaskNotify(lvgl_task_.handle(), task_notify_restarting_bit, eSetBits);
        break;
    case CONFIG_CHANGE:
        xTaskNotify(lvgl_task_.handle(), config_changed_bit, eSetBits);
        break;
    case DEVICE_IDENTIFY:
        xTaskNotify(lvgl_task_.handle(), idenitfy_device_bit, eSetBits);
        break;
    }
}
