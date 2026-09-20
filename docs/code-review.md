# Firmware review and fixes

Reviewed baseline: `e115bdc` (main), September 19, 2026. Scope: application code,
ILI9341/XPT2046 drivers, LVGL integration, BME680 wrapper, CubeMX configuration,
CubeIDE project settings and repository automation in all four firmware variants.
The legacy directory contains SquareLine design assets, not another firmware build.
Bundled HAL/LVGL/Bosch code was inspected where needed to confirm integration
behavior; this is not an exhaustive audit of those dependencies.

## Findings fixed

References below identify the **original baseline**, before these changes. Shared
issues apply to all four firmware variants unless a narrower scope is given.

1. **[MEMORY SAFETY] CRITICAL — PWM slider overwrites a four-byte buffer.**
   `variants/pwm-slider/Drivers/ui/ui_events.c:13,18`: even the shortest formatted
   label exceeds the buffer. Both active and archived UI exports now use a
   32-byte buffer and `snprintf`. Active PWM duty follows ARR rather than assuming
   a fixed timer period. Host tests cover negative, nominal and over-range values.
2. **[CONCURRENCY] CRITICAL — LVGL releases DMA source memory too early.**
   `variants/pwm-slider/Core/Src/LCDController.c:153,156`: flush-ready immediately
   follows DMA submission, permitting buffer reuse and another SPI transaction.
   The display now registers LVGL's flush-wait callback; completion/error callbacks
   publish state, and foreground code waits or aborts before releasing the buffer.
   CS is released after completion/recovery. Tests cover ownership, start failures,
   errors, missing completion, unrelated SPI callbacks and tick wraparound.
3. **[INTERRUPT SAFETY] CRITICAL — HAL timeout cannot advance inside DMA IRQ.**
   `variants/pwm-slider/Core/Inc/stm32f4xx_hal_conf.h:151` sets SysTick priority 15,
   while `Core/Src/main.c:429` sets DMA priority 0. The bundled
   `Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c:2707` waits on SPI flags
   using the HAL tick inside DMA completion. A stalled flag can trap that IRQ.
   SysTick now has priority 0, SPI/DMA priority 5, in both code and CubeMX files.
4. **[PROTOCOL] WARNING — Touch samples lose one significant bit.**
   `variants/pwm-slider/Core/Src/XPT2046.c:17`: shifting by four halves the 12-bit
   sample. The 24-clock transfer now discards three trailing bits and masks the
   12-bit result. Default calibration values were scaled accordingly. The SPI2
   prescaler is now 32 (1.40625 MHz with the existing 45 MHz APB1 clock), replacing
   5.625 MHz. Reference: [XPT2046 datasheet, electrical characteristics and Figure 12](https://files.waveshare.com/wiki/common/XPT2046_Datasheet.pdf).
5. **[RESOURCE CONFIGURATION] WARNING — Touch requires an undocumented IRQ wire.**
   `variants/pwm-slider/Core/Inc/XPT2046.h:17` and `Core/Src/XPT2046.c:25` read PB4,
   although the README specifies no PENIRQ connection. All variants now poll
   pressure, disable the unused EXTI interrupt and pull PB4 up. Chip selects start
   inactive in both generated GPIO initialization and CubeMX settings.
6. **[COORDINATES] WARNING — Portrait bounds are used for landscape touch.**
   `variants/lvgl-start/Core/Src/TouchController.c:54,55` and the squareline-base
   mapping produce Y values outside the 240-pixel display and cannot reach its
   full 320-pixel width. Shared mapping now uses display dimensions, clamps to
   resolution minus one and supports either raw-axis direction.
7. **[CALIBRATION] WARNING — Startup calibration neither applies nor times out.**
   `variants/pwm-slider/Core/Src/TouchController.c:71,115` only prints readings,
   blocks startup indefinitely without touch, and retains the temporary screen.
   Calibration is now optional, applies the inset corner measurements to RAM,
   rejects degenerate spans, times out each press/release and restores/deletes
   the temporary screen. Hardware accuracy and the interactive flow need board
   testing; host coverage exercises the mapping math.
8. **[DISPLAY PROTOCOL] WARNING — Fill geometry and transfer lengths are wrong.**
   `variants/pwm-slider/Core/Src/ILI9341.c:126,196,205`: fill uses portrait bounds
   after landscape rotation; a full-frame byte count truncates to HAL's 16-bit
   length. Bounds now follow rotation, endpoints use both bytes, blocking writes
   split large transfers, and DMA rejects transfers above 65534 bytes. The formerly
   declared-only DrawPixel function is implemented. Full-frame and window-byte
   tests cover these paths.
9. **[HAL CORRECTNESS] WARNING — Failed transfers appear successful or can stall.**
   `variants/pwm-slider/Core/Src/XPT2046.c:16` ignores HAL results and can consume
   uninitialized receive bytes. Display commands also ignore failures. Transfers
   now have finite timeouts, touch outputs remain unchanged on failure, display
   APIs return status, and LVGL initialization/flush failures reach Error_Handler.
   A display fault deliberately stops the application after DMA recovery; it
   does not silently discard a frame or promise automatic reconnection.
10. **[RESPONSIVENESS] WARNING — BME680 conversions block the GUI loop.**
    `variants/bme680/Core/Src/bme68xController.c:111` sleeps through every heater
    and conversion interval. ReadData now starts/polls conversion and returns
    BME68X_W_NO_NEW_DATA while pending, allowing LVGL to run between polls. I2C
    timeouts are finite. Tests cover pending/completed/error states and tick wrap.
11. **[SENSOR PRESENTATION] WARNING — History order and gas validity are ignored.**
    `variants/bme680/Core/Src/main.c:237,244`: manual array wrapping does not
    advance LVGL's chart start index, and gas values are displayed before checking
    heater stability. Chart updates now use lv_chart_set_next_value; invalid or
    unstable gas samples show “Warming up”. Raw resistance thresholds remain
    illustrative, not a calibrated air-quality index.
12. **[BUILD/CI] WARNING — CI is invalid and Release include paths are incomplete.**
    `.github/workflows/repository-hygiene.yml:1` contains literal backslash-n
    characters rather than YAML newlines. CubeIDE Release include lists omit LVGL,
    UI and (where needed) sensor directories. The workflow now runs host tests,
    parity checks and four ARM builds; Release includes match Debug. A portable
    CMake build produces ELF/HEX/BIN/map files. Compiling squareline-base also
    exposed an undeclared debug printf; the unused debug print was removed.

**Totals: 3 CRITICAL, 9 WARNING, 0 INFO; all 12 findings addressed.**

## Validation

- PASS: all four firmware images compiled and linked using ARM GNU 15.3.1,
  Cortex-M4 hard-float, with section garbage collection.
- PASS: five host CTest suites using Homebrew LLVM with AddressSanitizer and
  UndefinedBehaviorSanitizer; separate unsanitized tests also passed.
- PASS: shared driver parity, CubeMX consistency, repository hygiene and diff
  whitespace checks (accounting for CubeMX's existing CRLF files).
- Apple Command Line Tools' sanitizer runtime hung before main on this host,
  including outside the sandbox. Homebrew LLVM's runtime completed all tests.
- NOT RUN: STM32CubeIDE GUI builds or CubeMX regeneration. Release include
  changes were inspected; CMake builds provide the executed compiler evidence.
- NOT RUN: hardware display/touch calibration, sensor disconnect recovery,
  logic-analyzer timing or sustained DMA operation. See README's board checks.

## Remaining limits

The firmware still uses its bundled LVGL 9.2.3-dev snapshot and existing ST/Bosch
libraries. Upgrading those wholesale would require a separate compatibility and
hardware validation pass, particularly for SquareLine exports and RGB565 byte
swapping. No claim is made that these dependencies are current or fully audited.
The existing HSE oscillator mode and 45 MHz display SPI clock are preserved;
verify oscillator wiring and panel/module timing on the actual board. The driver
API assumes one bare-metal foreground owner, not concurrent RTOS callers. LVGL
continues using its configured fixed heap; this review does not prohibit LVGL's
internal allocations. The application remains fail-stop for display failures.
