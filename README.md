How to build PlatformIO based project
=====================================

1. [Install PlatformIO Core](https://docs.platformio.org/page/core.html)
2. Download [development platform with examples](https://github.com/platformio/platform-espressif32/archive/develop.zip)
3. Extract ZIP archive
4. Run these commands:

```shell
# Change directory to example
$ cd platform-espressif32/examples/arduino-wifiscan

# Build project
$ pio run

# Upload firmware
$ pio run --target upload

# Build specific environment
$ pio run -e quantum

# Upload firmware for the specific environment
$ pio run -e quantum --target upload

# Clean build files
$ pio run --target clean
```

How to debug ESP32 S3
1. Connect to usb port that supports JTAG debug
2. Download tool from https://zadig.akeo.ie/downloads/#google_vignette
3. Open it, click Options -> List All Devices -> Select USB JTAG/serial debug unit (Interface 2)
4. Change driver to libusbK with arrows and click install button.
5. Add to platformio.ini
```
debug_tool = esp-builtin
debug_init_break = tbreak setup
```
Can be usefull https://community.platformio.org/t/cannot-run-builtin-debugger-on-esp32-s3-board/36384/4