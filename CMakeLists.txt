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
set(CMAKE_PROJECT_NAME u5a9_lvgl)

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

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    platform/display/board_lcd.c
    platform/display/framebuffer.c
    platform/display/lv_port_display.c
    platform/os/freertos_runtime_stats.c
    platform/perf/perf_profiler.c
    platform/perf/perf_gpio.c
    platform/graphics/lv_draw_dma2d_u5.c
    platform/memory/mpu.c
    platform/memory/hspi_psram.c
    platform/memory/ospi_nor.c
    platform/perf/perf_uart.c
    platform/perf/perf_nema_alloc.c
    Src/hspi1.c
    Src/octospi1.c
    Src/usart1.c
    benchmarks/bench_runner.c
    benchmarks/bench_full_repaint.c
    benchmarks/bench_transform.c
    benchmarks/bench_alpha_layers.c
    benchmarks/bench_text_scroll.c
    benchmarks/bench_mixed.c
    BSP/STM32U5x9J-DK/stm32u5x9j_discovery_hspi.c
    BSP/STM32U5x9J-DK/stm32u5x9j_discovery_ospi.c
    BSP/Components/aps512xx/aps512xx.c
    BSP/Components/mx25um51245g/mx25um51245g.c
)

# LVGL v9.3.0 (official release tree, configured via config/lv_conf.h)
set(LV_BUILD_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config/lv_conf.h CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/LVGL)

# LV_OS_FREERTOS needs kernel headers inside lvgl TUs (lv_freertos.h);
# Nema STM32 HAL glue needs the device HAL headers
target_include_directories(lvgl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33_NTZ/non_secure
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/perf
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/LVGL/libs/nema_gfx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/LVGL/src
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32U5xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32U5xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32U5xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include
)
target_compile_definitions(lvgl PUBLIC STM32U5A9xx USE_HAL_DRIVER)

# NemaGFX precompiled library (cortex_m33_revC, hard-float; plan §7 M4)
set(NEMA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/LVGL/libs/nema_gfx/lib/core/cortex_m33_revC/gcc/libnemagfx-float-abi-hard.a)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/display
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/perf
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/os
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/memory
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/LVGL/src
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/STM32U5x9J-DK
)

# Optional perf probe module (plan §5.2: default off, never in acceptance)
option(PERF_GPIO_EN "Optional perf GPIO probe module" OFF)
if(PERF_GPIO_EN)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PERF_GPIO_EN=1)
endif()

# Optional PSRAM 64 MiB full-coverage boot diagnostic (plan §7 M5 上板验证 #1)
option(PSRAM_FULL_DIAG "PSRAM full-walk boot diagnostic (delays boot ~10-20 s)" OFF)
if(PSRAM_FULL_DIAG)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PSRAM_FULL_DIAG=1)
endif()

# Benchmark scene selection (plan §5.3; default mixed = legacy demo behavior)
set(BENCH_SCENE "mixed" CACHE STRING "benchmark scene: mixed|full_repaint|transform|alpha|text")
if(BENCH_SCENE STREQUAL "full_repaint")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BENCH_SCENE_FULL_REPAINT=1)
elseif(BENCH_SCENE STREQUAL "transform")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BENCH_SCENE_TRANSFORM=1)
elseif(BENCH_SCENE STREQUAL "alpha")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BENCH_SCENE_ALPHA=1)
elseif(BENCH_SCENE STREQUAL "text")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BENCH_SCENE_TEXT=1)
endif()

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Nema allocator statistics via symbol wrapping (plan §7 M5; map must show __wrap bindings)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wl,--wrap=tsi_malloc_pool
    -Wl,--wrap=tsi_free
)

# Add linked libraries (--group: lvgl and the precompiled NemaGFX library
# reference each other; single-pass archive scan would miss the back-references)
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    -Wl,--start-group
    lvgl::lvgl
    ${NEMA_LIB}
    m
    -Wl,--end-group

    # Add user defined libraries
)
