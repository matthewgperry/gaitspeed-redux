# Gait Speed Clinic Device (RTOS Redux)

New firmware for the Gait Speed Clinic Device.

This repository contains the firmware for the new system architecture:

- Two Display/Main Boards
- Two Sensor Boards
- Boards connected on CAN bus

## Project Structure

```
gaitspeed-rtos/
├── sensor_board/   # STM32C092FBP — distance sensor node
└── display_board/  # STM32F103RC  — main display and touch UI
```

As a continuation of the previous firmware, these directory contain C++ files which use the `Arduino.h` header.
If coming from the previous firmware in the Arduino IDE, the main difference is `.ino` files are not used and PlatformIO manages the framework and library dependencies.

## Building and Uploading

Each board is a separate PlatformIO project.

To compile and flash to the boards, one needs:

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) or the PlatformIO IDE extension
- ST-Link programmer/debugger (or CMSIS-DAP programmer)

Run the following commands (or IDE extension buttons) from within the respective subdirectory.

### Sensor Board

```sh
cd sensor_board
pio run                          # build
pio run --target upload          # build and upload via ST-Link
pio device monitor               # open serial monitor (115200 baud)
```

### Display Board

```sh
cd display_board
pio run                          # build
pio run --target upload          # build and upload via ST-Link
pio device monitor               # open serial monitor (115200 baud)
```

To run the native unit tests for the display board:

```sh
cd display_board
pio test -e native
```

## Custom Board Support (Sensor Board)

The sensor board uses an STM32C092FBP, which is supported by the STM32 Arduino core but not yet by PlatformIO's bundled board definitions. Three files must be present in the PlatformIO package directory:

```
~/.platformio/packages/framework-arduinoststm32/
├── variants/STM32C0xx/C092ECY_C092F(B-C)P/ldscript.ld
├── variants/STM32C0xx/C092ECY_C092F(B-C)P/PeripheralPins.c
└── libraries/SrcWrapper/inc/uart.h
```

These files are located in this repo in `/board_def`
PlatformIO's configuration directory can vary by system and installation method.
Check the [documentation](https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/core_dir.html) for where it is one your system.

**Warning:** These files will be removed if PlatformIO updates the `framework-arduinoststm32` package. Re-apply them after any framework update.
