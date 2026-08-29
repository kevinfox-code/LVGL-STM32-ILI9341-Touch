# LVGL STM32 ILI9341 Touch

Standalone LVGL/SquareLine projects for the STM32F446RE Nucleo board with an
ILI9341 320x240 SPI TFT and XPT2046 resistive touchscreen. The variants share
the same board, display, touch controller, and wiring; they differ in the
application shown on the screen.

## Variants

- [`variants/bme680`](variants/bme680): SquareLine/LVGL interface with BME680
  environmental-sensor integration.
- [`variants/pwm-slider`](variants/pwm-slider): SquareLine/LVGL PWM slider
  interface.
- [`variants/squareline-base`](variants/squareline-base): Original STM32F446RE
  SquareLine/LVGL base project.
- [`variants/lvgl-start`](variants/lvgl-start): Bare-metal LVGL touchscreen
  starter/demo project.
- [`variants/pwm-slider-legacy`](variants/pwm-slider-legacy): Earlier PWM
  slider project retained for comparison and migration.

## Shared wiring

### ILI9341 display — SPI1

| Display pin | STM32F446RE pin |
| --- | --- |
| CS | PA9 |
| RESET | PC7 |
| DC | PB6 |
| MOSI | PA7 / SPI1_MOSI |
| SCK | PA5 / SPI1_SCK |
| MISO | PA6 / SPI1_MISO |
| LED | 3.3V |

### XPT2046 touchscreen — SPI2

| Touch pin | STM32F446RE pin |
| --- | --- |
| T_CS | PA8 |
| T_CLK | PB10 / SPI2_SCK |
| T_DIN | PC1 / SPI2_MOSI |
| T_DO | PC2 / SPI2_MISO |
| T_IRQ | Not connected; polling is used |

Both projects target `STM32F446RETx` and are intended to be opened and built
with STM32CubeIDE. Each variant retains its original CubeMX project files and
generated drivers.

## License

See the license files included with each variant and its bundled dependencies.
