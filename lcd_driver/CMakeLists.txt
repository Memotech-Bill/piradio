#  This file is for using the SDK to create software
#  that runs on the Raspberry Pi Pico.  Please set
#
#     PICO_SDK_PATH
#
#  to the subdirectory where you have installed the SDK. */

set(PICO_BOARD pico)

cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(lcd_driver C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC
  -DPROMPT=0
  -DLCD_SIZE=20
  -DBUFF_LEN=256
  -DSCROLL_RATE=200
  -DSCROLL_PAUSE=2000
  -DPICO_DEFAULT_UART=0
  -DPICO_DEFAULT_UART_TX_PIN=16
  -DPICO_DEFAULT_UART_RX_PIN=17
  )

target_compile_options(${PROJECT_NAME} PRIVATE -g)
target_link_options(${PROJECT_NAME} PRIVATE -g)

target_sources(${PROJECT_NAME} PRIVATE
  lcd_driver.c
  )

pico_generate_pio_header(${PROJECT_NAME}
  ${CMAKE_CURRENT_LIST_DIR}/lcd_driver.pio
  )

target_link_libraries(${PROJECT_NAME} PRIVATE
  pico_stdlib
  pico_stdio
  pico_stdio_usb
  pico_time
  hardware_pio
  hardware_timer
  )

pico_add_extra_outputs(${PROJECT_NAME})
