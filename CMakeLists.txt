cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME firsth7)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# ---- LVGL (built as static library) ----
set(LV_CONF_PATH "${CMAKE_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
add_subdirectory(lvgl)
# Propagate cross-compile flags to LVGL library
target_compile_options(lvgl PRIVATE
    -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -fdata-sections -ffunction-sections -Os
)
# LVGL needs STM32 HAL headers (lv_conf.h references stm32h7xx_hal.h for tick)
target_link_libraries(lvgl PUBLIC stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # BSP LCD driver
    BSP/LCD_ST7789/lcd_core.c
    BSP/LCD_ST7789/lcd_port.c
    BSP/LCD_ST7789/lcd_font.c
    BSP/LCD_ST7789/lcd_brightness.c
    BSP/LCD_ST7789/ft6336.c
    BSP/LCD_ST7789/touch.c
    BSP/LCD_ST7789/lv_port_disp.c
    BSP/LCD_ST7789/lv_port_indev.c
    BSP/LCD_ST7789/lv_font_custom_cn16.c
    # Application UI
    calculator/calculator_app.c
    clock/clock_app.c
    ui/ui.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}              # for lv_conf.h
    ${CMAKE_SOURCE_DIR}/lvgl         # for lvgl.h etc.
    ${CMAKE_SOURCE_DIR}/BSP/LCD_ST7789
    ${CMAKE_SOURCE_DIR}/calculator
    ${CMAKE_SOURCE_DIR}/clock
    ${CMAKE_SOURCE_DIR}/ui
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lvgl
    # Add user defined libraries
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary -j .isr_vector -j .text -j .rodata -j .ARM.extab -j .ARM -j .preinit_array -j .init_array -j .fini_array -j .data $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}_internal.bin
    COMMAND ${CMAKE_OBJCOPY} -O binary -j .extflash $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}_qspi.bin
    BYPRODUCTS ${CMAKE_PROJECT_NAME}_internal.bin ${CMAKE_PROJECT_NAME}_qspi.bin
    COMMENT "Generating internal flash and QSPI flash binary images"
)
