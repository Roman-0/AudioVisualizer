cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico2 CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(audioviz C CXX ASM)

pico_sdk_init()

add_executable(audioviz
    main.c
    src/Display.c
    lib/kiss_fft.c
)

target_compile_definitions(audioviz PRIVATE KISS_FFT_FLOAT=1)

target_link_libraries(audioviz
    pico_stdlib
    hardware_spi
    hardware_adc
    hardware_dma
)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(audioviz pico_cyw43_arch_none)
endif()

pico_add_extra_outputs(audioviz)
pico_enable_stdio_usb(audioviz 1)
pico_enable_stdio_uart(audioviz 0)