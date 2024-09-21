#pragma once

#include "util/noncopyable.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX final : public lgfx::LGFX_Device, esp32::noncopyable
{
  public:
    LGFX();

    const uint16_t Width = 240;
    const uint16_t Height = 320;

  private:
    lgfx::Panel_ST7789 panel_instance_;
    lgfx::Bus_SPI bus_instance_;
};
