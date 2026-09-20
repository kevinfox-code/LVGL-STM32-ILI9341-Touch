#!/usr/bin/env python3
"""Keep standalone copies of the reviewed drivers and CubeMX settings aligned."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
VARIANTS = ("bme680", "pwm-slider", "squareline-base", "lvgl-start")
SHARED = (
    "Core/Src/ILI9341.c", "Core/Inc/ILI9341.h", "Core/Src/LCDController.c",
    "Core/Src/XPT2046.c", "Core/Inc/XPT2046.h", "Core/Src/TouchController.c",
    "Core/Inc/TouchController.h", "Core/Src/TouchCalibration.c", "Core/Inc/TouchCalibration.h",
)
assert (ROOT / "README.md").stat().st_size > 0
for relative in SHARED:
    baseline = (ROOT / "variants" / VARIANTS[0] / relative).read_bytes()
    for variant in VARIANTS[1:]:
        assert (ROOT / "variants" / variant / relative).read_bytes() == baseline, (variant, relative)
for variant in VARIANTS:
    directory = ROOT / "variants" / variant
    config = next(directory.glob("*.ioc")).read_text()
    main = (directory / "Core/Src/main.c").read_text()
    assert "SPI2.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_32" in config
    assert "hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;" in main
    assert "PB4.Signal=GPIO_Input" in config
    assert "GPIOA, T_CS_Pin|CS_Pin, GPIO_PIN_SET" in main
tracked = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT).decode().split("\0")
assert not any(Path(name).name in (".DS_Store", "Thumbs.db") for name in tracked)
for variant in VARIANTS:
    directory = ROOT / "variants" / variant
    config = next(directory.glob("*.ioc")).read_text()
    hal = (directory / "Core/Inc/stm32f4xx_hal_conf.h").read_text()
    assert "NVIC.SysTick_IRQn=true\\:0\\:0" in config
    assert "NVIC.DMA2_Stream3_IRQn=true\\:5\\:0" in config
    assert "TICK_INT_PRIORITY            0U" in hal
print("PASS: repository hygiene, shared driver parity and CubeMX settings")
