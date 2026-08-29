# LVGL STM32F446RE Touchscreen Demo

A project using the **STM32F446RE Nucleo board** with an **ILI9341-based 320x240 SPI TFT display** and **XPT2046 resistive touch controller**, powered by **LVGL v9.2**. Built with **STM32CubeIDE** on macOS.

## Features

* LVGL 9.2.3-dev (custom-configured)
* SPI-based **ILI9341** driver (landscape mode)
* **XPT2046** touchscreen driver on SPI2
* LVGL input device integration (via `TouchController.c/.h`)
* LVGL output handler (via `LCDController.c/.h`)
* Label/button GUI with full touch interaction
* Bare-metal (no RTOS)
* Manual pin assignments with CubeMX-generated setup
* Compatible with `HAL_SPI_Transmit()` and optional DMA

---

## Touchscreen Test Demo

Displays a **"Touch Me"** button. When pressed, the top label changes text to:

> **"Touched!"**

This confirms full touch input and LVGL event flow.

---

## Pinout

### ILI9341 Display (SPI1)

| ILI9341 Pin | Function     | STM32 Pin                 |
| ----------- | ------------ | ------------------------- |
| `VCC`       | 3.3V         | 5V (OK for many modules)  |
| `GND`       | Ground       | GND                       |
| `CS`        | Chip Select  | `PA9`                     |
| `RESET`     | Reset        | `PC7`                     |
| `DC`        | Data/Command | `PB6`                     |
| `SDI/MOSI`  | SPI MOSI     | `PA7` (SPI1\_MOSI)        |
| `SCK`       | SPI Clock    | `PA5` (SPI1\_SCK)         |
| `SDO/MISO`  | SPI MISO     | `PA6` (SPI1\_MISO)        |
| `LED`       | Backlight    | 3.3V                      |

### XPT2046 Touch Controller (SPI2)

| XPT2046 Pin | Function    | STM32 Pin           |
| ----------- | ----------- | ------------------- |
| `T_IRQ`     | Touch IRQ   | *Not connected*     |
| `T_CS`      | Chip Select | `PA8`               |
| `T_CLK`     | Clock       | `PB10` (SPI2\_SCK)  |
| `T_DIN`     | MOSI        | `PC1` (SPI2\_MOSI) |
| `T_DO`      | MISO        | `PC2` (SPI2\_MISO) |

---

## Setup

### CubeIDE Configuration

* **MCU:** `STM32F446RETx`
* **Clock:** PLL from HSE (180 MHz)
* **SPI1:** For ILI9341 (8-bit, fast prescaler)
* **SPI2:** For XPT2046 (lower speed prescaler)
* **DMA:** Optional for SPI1 TX
* **GPIO:** CS/DC/RESET/T\_CS manually assigned

### LVGL Configuration

`lv_conf.h` highlights:

* `LV_COLOR_DEPTH 16` (RGB565)
* `LV_USE_DRAW_SW 1`
* `LV_USE_THEME_DEFAULT 1`
* `LV_THEME_DEFAULT_DARK 0`
* `LV_USE_INPUT_DEVICE 1`
* Touch input via `lv_indev_set_read_cb`

---

## Project Structure

```txt
LVGL_Start_STM32F446RE/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── ILI9341.h
│   │   ├── XPT2046.h
│   │   ├── TouchController.h    ← LVGL input integration
│   │   └── LCDController.h      ← LVGL output integration
│   ├── Src/
│   │   ├── main.c
│   │   ├── ILI9341.c
│   │   ├── XPT2046.c            ← Raw touch SPI driver
│   │   ├── TouchController.c    ← LVGL input glue logic
│   │   └── LCDController.c      ← LVGL output glue logic
├── Drivers/
│   ├── CMSIS/
│   ├── STM32F4xx_HAL_Driver/
│   └── lvgl/
├── lv_conf.h
├── makefile
└── README.md
```

---

## How to Build & Run

1. Open with STM32CubeIDE
   * Import the project into STM32CubeIDE
   * Ensure all paths are correctly set up
   * Check `lv_conf.h` for any custom LVGL settings
2. Build and flash the project
3. Screen should show:

   * **Top label:** "Touch the screen"
   * **Centered button:** "Touch Me"
4. Touch the button — label updates to say "Touched!"

---

## Notes

* The XPT2046 uses SPI polling (no IRQ)
* Touch coordinates are calibrated to match the display resolution
* Touch reads on demand during `lv_timer_handler`
* Default rotation: landscape (ILI9341 `SetRotation(3)`)

---

## License & Author

MIT License
**Kevin Fox**
