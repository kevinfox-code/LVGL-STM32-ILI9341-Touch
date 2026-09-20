cmake_minimum_required(VERSION 3.20)
project(lvgl_stm32 C ASM)
set(VARIANT "pwm-slider" CACHE STRING "Firmware variant")
set_property(CACHE VARIANT PROPERTY STRINGS bme680 pwm-slider squareline-base lvgl-start)
if(NOT VARIANT MATCHES "^(bme680|pwm-slider|squareline-base|lvgl-start)$")
    message(FATAL_ERROR "Unknown firmware variant: ${VARIANT}")
endif()
set(V "${CMAKE_CURRENT_SOURCE_DIR}/variants/${VARIANT}")
file(GLOB CORE CONFIGURE_DEPENDS "${V}/Core/Src/*.c")
file(GLOB HAL CONFIGURE_DEPENDS "${V}/Drivers/STM32F4xx_HAL_Driver/Src/*.c")
file(GLOB_RECURSE LVGL CONFIGURE_DEPENDS "${V}/Drivers/lvgl/src/*.c")
file(GLOB_RECURSE UI CONFIGURE_DEPENDS "${V}/Drivers/ui/*.c")
file(GLOB SENSOR CONFIGURE_DEPENDS "${V}/Drivers/BME68x/*.c")
add_executable(firmware ${CORE} ${HAL} ${LVGL} ${UI} ${SENSOR}
    "${V}/Core/Startup/startup_stm32f446retx.s")
set_target_properties(firmware PROPERTIES SUFFIX ".elf" C_STANDARD 11 C_STANDARD_REQUIRED YES)
target_compile_definitions(firmware PRIVATE USE_HAL_DRIVER STM32F446xx LV_CONF_INCLUDE_SIMPLE)
target_include_directories(firmware PRIVATE
    "${V}/Core/Inc" "${V}/Drivers" "${V}/Drivers/lvgl" "${V}/Drivers/ui"
    "${V}/Drivers/BME68x" "${V}/Drivers/STM32F4xx_HAL_Driver/Inc"
    "${V}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy"
    "${V}/Drivers/CMSIS/Device/ST/STM32F4xx/Include" "${V}/Drivers/CMSIS/Include")
set(MCU -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
target_compile_options(firmware PRIVATE ${MCU} -Os -g3 -ffunction-sections -fdata-sections -Wall -Wextra)
target_link_options(firmware PRIVATE ${MCU} "-T${V}/STM32F446RETX_FLASH.ld"
    --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/firmware.map" -Wl,--print-memory-usage -u _printf_float)
target_link_libraries(firmware PRIVATE m)
set_property(TARGET firmware APPEND PROPERTY LINK_DEPENDS "${V}/STM32F446RETX_FLASH.ld")
find_program(OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(SIZE arm-none-eabi-size REQUIRED)
add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${OBJCOPY} -O ihex $<TARGET_FILE:firmware> firmware.hex
    COMMAND ${OBJCOPY} -O binary $<TARGET_FILE:firmware> firmware.bin
    COMMAND ${SIZE} $<TARGET_FILE:firmware>)
