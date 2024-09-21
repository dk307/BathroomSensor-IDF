#pragma once

#include "led_strip.h"
#include "util/exceptions.h"

class inbuild_led
{
  public:
    void init()
    {
        led_strip_config_t strip_config{};
        strip_config.strip_gpio_num = GPIO_NUM_48;
        strip_config.max_leds = 1;
        strip_config.led_pixel_format = LED_PIXEL_FORMAT_GRB;
        strip_config.led_model = LED_MODEL_WS2812;
        strip_config.flags.invert_out = false;

        led_strip_rmt_config_t rmt_config{};
        rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
        rmt_config.resolution_hz = 10 * 1000 * 1000; // 10MH
        rmt_config.flags.with_dma = false;

        CHECK_THROW_ESP(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_));

        /* Set all LED off to clear all pixels */
        CHECK_THROW_ESP(led_strip_clear(led_strip_));
    }

    void set_color(uint32_t red, uint32_t green, uint32_t blue)
    {
        CHECK_THROW_ESP(led_strip_set_pixel(led_strip_, 0, red, green, blue));
        /* Refresh the strip to send data */
        CHECK_THROW_ESP(led_strip_refresh(led_strip_));
    }

    void clear()
    {
        /* Set all LED off to clear all pixels */
        CHECK_THROW_ESP(led_strip_clear(led_strip_));
    }

  private:
    led_strip_handle_t led_strip_{};
};