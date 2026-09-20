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
- [`variants/pwm-slider-legacy`](variants/pwm-slider-legacy): Earlier SquareLine PWM
  design assets retained for comparison; this directory is not a firmware build.

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

All four firmware projects target `STM32F446RETx` and are intended to be opened and built
with STM32CubeIDE. Each variant retains its original CubeMX project files and
generated drivers.

## Build and test

STM32CubeIDE remains supported: import the desired variant's `.project`. The
active UI sources live in `Drivers/ui`; other UI export directories are archives.
A command-line build requires CMake 3.20+, a build tool and ARM GNU with newlib:

```sh
cmake -S . -B build/pwm-slider \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DVARIANT=pwm-slider
cmake --build build/pwm-slider --parallel
```

Use `bme680`, `squareline-base` or `lvgl-start` for the other images. Outputs are
`firmware.elf`, `.hex`, `.bin` and `.map` inside the build directory. If multiple
ARM toolchains are installed, pass `-DCMAKE_C_COMPILER=/absolute/path/to/arm-none-eabi-gcc`
on the first configure. A compiler-only installation without newlib is insufficient.

Host regression tests use a native C compiler and mocked HAL/LVGL/Bosch calls:

```sh
cmake -S tests -B build/tests
cmake --build build/tests --parallel
ctest --test-dir build/tests --timeout 30 --output-on-failure
python3 scripts/check_repository.py
```

AddressSanitizer and UndefinedBehaviorSanitizer are enabled by default; use
`-DSANITIZE=OFF` only where unavailable. On the review Mac, Apple Clang's sanitizer
runtime hung before main; Homebrew LLVM worked with
`-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang` in a fresh build directory.
CI runs sanitized Linux host tests and builds all four firmware variants.

## Runtime behavior and calibration

The display uses two 10-row RGB565 buffers. LVGL waits for SPI1 DMA completion
before reusing a buffer; a 100 ms wait timeout aborts DMA before releasing its
source. HAL display errors enter `Error_Handler`. These drivers have one
foreground owner; call them outside interrupts and LVGL callbacks unless the
callback is the supplied display flush implementation. SysTick must retain a
higher preemption priority than SPI/DMA so HAL timeout checks can advance.

Touch uses SPI2 pressure polling; PB4/PENIRQ is optional and has a pull-up with
its interrupt disabled. SPI2 runs at approximately 1.406 MHz. Raw readings now
use the complete 12-bit range. Default mapping comes from the original panel's
measurements and may need adjustment for a different panel.

To calibrate, call `touch_calibrate()` after display/touch initialization from
the main context. Tap and release each green cross. The function returns true
when it applies the new mapping; each press/release has a 10-second timeout.
Failure retains the previous mapping, and either outcome restores the previous
screen. Calibration is held in RAM and is lost at reset. Startup does not force
calibration. Reapply matching changes if you regenerate the SquareLine event code.

The BME680 wrapper's `BME68x_ReadData` is now a start/poll API:
`BME68X_W_NO_NEW_DATA` means conversion is pending; keep servicing LVGL and poll
again. `BME68X_OK` with nonzero fields supplies a sample. The application displays
raw gas resistance, not a calibrated air-quality index.

## Review and board verification

See [the review report](docs/code-review.md) for findings, fixes and validation.
Shared driver copies are intentionally retained so each CubeIDE variant stays
standalone; the parity check prevents those copies from drifting.

Before using a build on hardware:

1. Confirm the existing 8 MHz HSE oscillator setup matches your Nucleo wiring.
2. Verify red/green/blue test colors and full-screen fills in landscape; qualify
   the existing 45 MHz SPI1 clock for your panel and wiring.
3. Check touch release, all four corners and repeated calibration, including
   timeout without a panel connected.
4. Exercise rapid redraws/PWM changes and verify PWM output at 0%, 50% and 100%.
5. On BME680, verify UI response during measurement, gas warm-up status and
   behavior after a sensor disconnect/reconnect.

No hardware tests were performed during this review. Vendored LVGL/HAL/Bosch
versions are retained; upgrading them requires separate compatibility testing.

## License

See the license files included with each variant and its bundled dependencies.
