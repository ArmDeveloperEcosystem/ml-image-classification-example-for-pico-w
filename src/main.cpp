#include <stdio.h>
#include <pico/stdlib.h>

#include <bsp/board.h>
#include <tusb.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "usb_descriptors.h"

extern "C" {
    #include "pico/hm01b0.h"
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

uint8_t monochrome_buffer[FRAME_WIDTH * FRAME_HEIGHT];

SemaphoreHandle_t tensor_arena_mutex;
uint8_t tensor_arena[101 * 1024];
ImageClassifier image_classifier(tensor_arena, sizeof(tensor_arena));

void main_task(void*);
void usb_task(void*);

int main(void)
{
    board_init();

    tud_init(BOARD_TUD_RHPORT);

    stdio_init_all();

    tensor_arena_mutex = xSemaphoreCreateMutex();

    xTaskCreate(main_task, "MainTask", 2 * 1024, NULL, (tskIDLE_PRIORITY + 1), NULL);
    xTaskCreate(usb_task, "USBTask", 2 * 1024, NULL, (tskIDLE_PRIORITY + 1), NULL);
    vTaskStartScheduler();

    return 0;
}

void main_task(void*)
{
    while (!stdio_usb_connected()) {
        taskYIELD();
    }

    printf("hello\n");

    if (hm01b0_init(&hm01b0_config) != 0) {
        printf("failed to initialize camera!\n");
        return;
    }

    if (!image_classifier.init()) {
        printf("failed to initialize image classifier!\n");
        return;
    }

    while (1) {
        xSemaphoreTake(tensor_arena_mutex, portMAX_DELAY);

        hm01b0_read_frame(monochrome_buffer, sizeof(monochrome_buffer));

        float* predictions = image_classifier.predict(monochrome_buffer, FRAME_WIDTH, FRAME_HEIGHT);

        printf("predictions = %f %f\n", predictions[0], predictions[1]);

        if (tud_video_n_streaming(0, 0)) {
            const uint8_t* src = monochrome_buffer;
            uint8_t* dst = tensor_arena;

            for (int y = 0; y < FRAME_HEIGHT; y++) {
                for (int x = 0; x < FRAME_WIDTH; x++) {
                    *dst++ = *src++;
                    *dst++ = 128;
                }
            }

            tud_video_n_frame_xfer(0, 0, tensor_arena, FRAME_WIDTH * FRAME_HEIGHT * 2);
        } else {
            xSemaphoreGive(tensor_arena_mutex);
        }
    }
}

void usb_task(void*)
{
    while (1) {
        taskYIELD();

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
