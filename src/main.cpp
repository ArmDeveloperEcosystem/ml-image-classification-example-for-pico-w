#include <stdio.h>
#include <pico/stdlib.h>

#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"


int main(void)
{
    board_init();

    tud_init(BOARD_TUD_RHPORT);

    stdio_init_all();

    while (!stdio_usb_connected()) {
        tud_task();
    }

    printf("hello\n");

    while (1) {
        tud_task();
    }

    return 0;
}

extern "C" {
    void tud_video_frame_xfer_complete_cb(uint_fast8_t /*ctl_idx*/, uint_fast8_t /*stm_idx*/) {}

    int tud_video_commit_cb(uint_fast8_t /*ctl_idx*/, uint_fast8_t /*stm_idx*/, video_probe_and_commit_control_t const* /*parameters*/)
    {
        return VIDEO_ERROR_NONE;
    }
}
