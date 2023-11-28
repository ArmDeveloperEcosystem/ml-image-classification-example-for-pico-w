cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(ml-image-classification-example-for-pico-w)

pico_sdk_init()

add_executable(ml-image-classification-example-for-pico-w
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/hm01b0-library-for-pico/examples/usb_camera/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/example-of-a-slackbot-for-pico-w/pico-sdk/https_client.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/example-of-a-slackbot-for-pico-w/pico-sdk/slack_client.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/example-of-a-slackbot-for-pico-w/pico-sdk/tls_client.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/example-of-a-slackbot-for-pico-w/pico-sdk/wss_client.c
)

target_include_directories(ml-image-classification-example-for-pico-w PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/lib/hm01b0-library-for-pico/examples/usb_camera
    ${CMAKE_CURRENT_LIST_DIR}/lib/example-of-a-slackbot-for-pico-w/pico-sdk
    ${CMAKE_CURRENT_LIST_DIR}/lib/example-of-a-slackbot-for-pico-w/pico-sdk/config
)

target_compile_definitions(ml-image-classification-example-for-pico-w PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    SLACK_APP_TOKEN=\"${SLACK_APP_TOKEN}\"
    SLACK_BOT_TOKEN=\"${SLACK_BOT_TOKEN}\"
)

target_link_libraries(ml-image-classification-example-for-pico-w PRIVATE
    tinyusb_device
    tinyusb_board
    pico_stdlib
    pico-tflmicro
    pico_hm01b0

    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_mbedtls
    pico_mbedtls
    FreeRTOS-Kernel-Heap3 # FreeRTOS kernel and dynamic heap
    coreHTTP
    cJSON
)

pico_enable_stdio_usb(ml-image-classification-example-for-pico-w 1)
pico_enable_stdio_uart(ml-image-classification-example-for-pico-w 0)

pico_add_extra_outputs(ml-image-classification-example-for-pico-w)

add_subdirectory("lib/pico-tflmicro" EXCLUDE_FROM_ALL)
add_subdirectory("lib/hm01b0-library-for-pico" EXCLUDE_FROM_ALL)
add_subdirectory("lib/example-of-a-slackbot-for-pico-w/pico-sdk" EXCLUDE_FROM_ALL)
