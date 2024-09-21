#include "hardware/display/lgfx_device.h"

LGFX::LGFX()
{
    {
        auto cfg = bus_instance_.config();

        cfg.spi_host = SPI3_HOST; // Select the SPI to use ESP32-S2,C3: SPI2_HOST or SPI3_HOST / ESP32: VSPI_HOST or HSPI_HOST
        // * With the ESP-IDF version upgrade, the description of VSPI_HOST and HSPI_HOST will be deprecated, so if an error occurs, please use
        // SPI2_HOST and SPI3_HOST instead.
        cfg.spi_mode = 3;                  // Set SPI communication mode (0 ~ 3)
        cfg.freq_write = 80000000;         // SPI clock during transmission (maximum 80MHz, rounded to 80MHz divided by an integer)
        cfg.freq_read = 80000000;          // SPI clock when receiving
        cfg.spi_3wire = true;              // Set true if receiving is done via MOSI pin
        cfg.use_lock = true;               // Set true to use transaction lock
        cfg.dma_channel = SPI_DMA_CH_AUTO; // Set the DMA channel to use (0=DMA not used / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=automatic setting)

        // * Due to the ESP-IDF version upgrade, SPI_DMA_CH_AUTO (automatic setting) is recommended for the DMA channel. Specifying 1ch or 2ch is not
        // recommended.
        // display -scl=6, sda = 7
        cfg.pin_sclk = 6;  // Set SPI SCLK pin number
        cfg.pin_mosi = 7;  // Set the SPI MOSI pin number
        cfg.pin_miso = -1; // Set SPI MISO pin number (-1 = disable)
        cfg.pin_dc = 15;    // Set SPI D/C pin number (-1 = disable)
                           // If you use the same SPI bus as the SD card, be sure to set MISO without omitting it.

        bus_instance_.config(cfg);              // Apply the settings to the bus.
        panel_instance_.setBus(&bus_instance_); // Sets the bus to the panel.
    }

    {                                        // Set display panel control.
        auto cfg = panel_instance_.config(); // Get the structure for display panel settings.

        cfg.pin_cs = 17;   // Pin number to which CS is connected (-1 = disable)
        cfg.pin_rst = 16;   // Pin number to which RST is connected (-1 = disable)
        cfg.pin_busy = -1; // Pin number to which BUSY is connected (-1 = disable)

        // * The following setting values ​​are general default values ​​set for each panel, so please comment out any items you are unsure of
        // and try again.

        cfg.panel_width = LGFX::Width;   // Actual displayable width
        cfg.panel_height = LGFX::Height; // Actual display height
        cfg.offset_x = 0;                // Panel X direction offset amount
        cfg.offset_y = 0;                // Panel Y direction offset amount
        cfg.offset_rotation = 0;         // Offset of rotation direction value 0~7 (4~7 are upside down)
        cfg.dummy_read_pixel = 8;        // Number of bits for dummy read before pixel reading
        cfg.dummy_read_bits = 1;         // Number of bits for dummy read before reading data other than pixels
        cfg.readable = true;             // Set to true if data can be read
        cfg.invert = true;               // Set to true if the brightness of the panel is inverted
        cfg.rgb_order = false;           // Set to true if the red and blue colors of the panel are swapped
        cfg.dlen_16bit = false;          // Set to true for panels that transmit data length in 16-bit units using 16-bit parallel or SPI
        cfg.bus_shared = false;          // Set to true if the bus is shared with the SD card (control the bus with drawJpgFile, etc.)

        panel_instance_.config(cfg);
    }

    setPanel(&panel_instance_); // Sets the panel to use.
}