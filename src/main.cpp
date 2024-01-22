#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>

#include <bsp/board_api.h>
#include <tusb.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "usb_descriptors.h"

extern "C" {
    #include "pico/hm01b0.h"
    #include "logging.h"
    #include "slack_client.h"
}

#include "image_classifier.h"

const struct hm01b0_config hm01b0_config = {
    .i2c           = PICO_DEFAULT_I2C_INSTANCE,
    .sda_pin       = PICO_DEFAULT_I2C_SDA_PIN,
    .scl_pin       = PICO_DEFAULT_I2C_SCL_PIN,

    .vsync_pin     = 6,
    .hsync_pin     = 7,
    .pclk_pin      = 8,
    .data_pin_base = 9,    // Base data pin
    .data_bits     = 1,    // Use only 1 pin for data
    .pio           = pio0,
    .pio_sm        = 0,
    .reset_pin     = -1,   // Not connected
    .mclk_pin      = -1,   // Not connected

    .width         = FRAME_WIDTH,
    .height        = FRAME_HEIGHT,
};

SemaphoreHandle_t tensor_arena_mutex;
uint8_t tensor_arena[101 * 1024] __attribute__((aligned(16)));
ImageClassifier image_classifier(tensor_arena, sizeof(tensor_arena));

// use start of tensor area for yuv image buffer
uint8_t* const yuv_buffer = tensor_arena;

// use after the yuv image buffer for monochrome image buffer
uint8_t* const monochrome_buffer = yuv_buffer + (FRAME_WIDTH * FRAME_HEIGHT * 2);

char slack_client_buf[3 * 1024];
slack_client_t slack_client;

void main_task(void*);
void usb_task(void*);

bool occupied = false;
float smoothed_occupied_prediction = 0.0;

int main(void)
{
    board_init();

    tud_init(BOARD_TUD_RHPORT);

    stdio_init_all();

    tensor_arena_mutex = xSemaphoreCreateMutex();

    xTaskCreate(main_task, "MainTask", 2 * 1024, NULL, (tskIDLE_PRIORITY + 1), NULL);
    xTaskCreate(usb_task, "USBTask", 1 * 1024, NULL, (tskIDLE_PRIORITY + 1), NULL);
    vTaskStartScheduler();

    return 0;
}

void main_task(void*)
{
    // wait up to 5 seconds for the USB serial to be opened
    while (!stdio_usb_connected() && (xTaskGetTickCount() / pdMS_TO_TICKS(1)) < 5000) {
        taskYIELD();
    }

    LogInfo(("Hello"));

    if (hm01b0_init(&hm01b0_config) != 0) {
        LogError(("Failed to initialize camera!\n"));
        return;
    }

    // lower the exposure
    hm01b0_set_coarse_integration(200);

    if (!image_classifier.init()) {
        LogError(("Failed to initialize image classifier!\n"));
        return;
    }

    if (cyw43_arch_init()) {
        LogError(("Failed to initialize Wi-Fi!"));
        return;
    }

    cyw43_arch_enable_sta_mode();

    LogInfo(("Connecting to Wi-Fi SSID '%s'", WIFI_SSID));
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        LogError(("Failed to connect to Wi-Fi SSID '%s' !", WIFI_SSID));
        return;
    } else {
        LogInfo(("Connected to Wi-Fi SSID '%s'", WIFI_SSID));
    }

     if (slack_client_init(&slack_client, SLACK_BOT_TOKEN, NULL, slack_client_buf, sizeof(slack_client_buf)) != 0) {
        LogError(("Failed to initialize Slack client!"));
        return;
    }

    slack_client_post_message(&slack_client, ":wave: Hello Slack", SLACK_CHANNEL);

    while (1) {
        xSemaphoreTake(tensor_arena_mutex, portMAX_DELAY);

        hm01b0_read_frame(monochrome_buffer, FRAME_WIDTH * FRAME_HEIGHT);

        if (tud_video_n_streaming(0, 0)) {
            const uint8_t* src = monochrome_buffer;
            uint8_t* dst = yuv_buffer;

            for (int i = 0; i < FRAME_HEIGHT * FRAME_WIDTH; i++) {
                *dst++ = *src++;
                *dst++ = 128;
            }

            tud_video_n_frame_xfer(0, 0, yuv_buffer, FRAME_WIDTH * FRAME_HEIGHT * 2);
        } else {
            xSemaphoreGive(tensor_arena_mutex);
        }

        xSemaphoreTake(tensor_arena_mutex, portMAX_DELAY);
        float* predictions = image_classifier.predict(monochrome_buffer, FRAME_WIDTH, FRAME_HEIGHT);
        xSemaphoreGive(tensor_arena_mutex);

        smoothed_occupied_prediction *= 0.8;
        smoothed_occupied_prediction += predictions[1] * 0.2;

        LogInfo(("predictions = %f %f %f %f, smoothed_occupied_prediction = %f", predictions[0], predictions[1], predictions[2], predictions[3], smoothed_occupied_prediction));

        if (smoothed_occupied_prediction > 0.5) {
            if (!occupied) {
                occupied = true;

                LogInfo(("EV Charger is occupied"));

                slack_client_post_message(&slack_client, ":redlight: EV charger 1 is occupied :redlight:", SLACK_CHANNEL);
            }
        } else {
            if (occupied) {
                occupied = false;

                LogInfo(("EV Charger is available"));

                slack_client_post_message(&slack_client, ":greenlight: EV charger 1 is available :greenlight:", SLACK_CHANNEL);
            }
        }
    }
}

void usb_task(void*)
{
    while (1) {
        vTaskDelay(1);

        tud_task();
    }
}

extern "C" {
    void tud_video_frame_xfer_complete_cb(uint_fast8_t /*ctl_idx*/, uint_fast8_t /*stm_idx*/)
    {
        xSemaphoreGive(tensor_arena_mutex);
    }

    int tud_video_commit_cb(uint_fast8_t /*ctl_idx*/, uint_fast8_t /*stm_idx*/, video_probe_and_commit_control_t const* /*parameters*/)
    {
        return VIDEO_ERROR_NONE;
    }
}
