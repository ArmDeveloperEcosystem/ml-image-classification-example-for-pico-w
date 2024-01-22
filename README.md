# ML Image Classification Example for Pico W

Report the occupany status of an EV Charger with a [Raspberry Pi Pico W](https://www.raspberrypi.com/products/raspberry-pi-pico/), [Himax HM01B0 based](https://www.himax.com.tw/products/cmos-image-sensor/always-on-vision-sensors/hm01b0/) camera module board, and the [Slack API](https://api.slack.com).

Requires a Slack Bot token.


## Hardware

 * [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
 * [Arducam HM01B0 Monochrome QVGA SPI Camera Module for Raspberry Pi Pico](https://www.arducam.com/product/arducam-hm01b0-qvga-spi-camera-module-for-raspberry-pi-pico-2/)

### Default Pinout

| HM01B0 | Raspberry Pi Pico W |
| ------ | -------------------------- |
| VCC | 3V3 |
| SCL | GPIO5 |
| SDA | GPIO4 |
| VSYNC | GPIO6 |
| HREF | GPIO7 |
| PCLK | GPIO8 |
| D0 | GPIO9 |
| GND | GND |

## Software

### Cloning

```sh
git clone --recurse-submodules https://github.com/ArmDeveloperEcosystem/ml-image-classification-example-for-pico-w.git
```

### Building

1. [Set up the Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
2. Set `PICO_SDK_PATH`
```sh
export PICO_SDK_PATH=/path/to/pico-sdk
```
3. Change directories:
```
ml-image-classification-example-for-pico-w
```
4. Create `build` dir, run `cmake` and `make`:
```
mkdir build

cd build

cmake .. \
    -DPICO_BOARD=pico_w \
    -DWIFI_SSID="<WIFI SSID>" \
    -DWIFI_PASSWORD="<Wi-Fi Password>" \
    -DSLACK_BOT_TOKEN="<Slack Bot Token>" \
    -DSLACK_CHANNEL="<Slack Channel>"


make
```

5. Copy example `ml-image-classification-example-for-pico-w.uf2` to Pico W when in BOOT mode.

## License

[MIT](LICENSE)

---

Disclaimer: This is not an official Arm product.
