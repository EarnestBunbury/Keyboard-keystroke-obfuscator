









# RP2040 Keystroke Obfuscator

Hardware-layer USB HID man-in-the-middle proxy designed to intercept raw keystrokes, buffer them in RAM, and output synthetic, randomized timing bursts to defeat behavioral typing telemetry.

## Status & Architecture

This repository contains the core C logic (`main.c`) for USB device/host passthrough on an RP2040 microcontroller. Because this operates at the hardware USB layer rather than the OS software layer, it requires physical wiring between a USB host interface (your computer) and a USB device interface (your keyboard), mediated by a dual-port RP2040 board (such as a Raspberry Pi Pico paired with a USB OTG breakout).

## Bill of Materials
* 1x Raspberry Pi Pico (RP2040)
* 1x USB Type-A female breakout connector (for the keyboard)
* 1x USB Type-A or Micro-B cable (to the host machine)
* Miscellaneous hookup wire for GPIO/VBUS/GND routing.

## Building and Flashing

### 1. Prerequisites (Linux Mint / Debian)
Install the ARM GNU toolchain and build utilities:
```bash
sudo apt update && sudo apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential git


git clone [https://github.com/raspberrypi/pico-sdk.git](https://github.com/raspberrypi/pico-sdk.git) ~/pico-sdk
cd ~/pico-sdk && git submodule update --init
export PICO_SDK_PATH=~/pico-sdk


git clone [https://github.com/EarnestBunbury/Keyboard-keystroke-obfuscator.git](https://github.com/EarnestBunbury/Keyboard-keystroke-obfuscator.git)
cd Keyboard-keystroke-obfuscator
mkdir build && cd build
cmake ..
make

     Flashing

    Unplug your RP2040.

    Hold down the BOOTSEL button while plugging the USB cable into your machine.

    Copy the compiled binary directly to the mounted bootloader drive:

cp firmware.uf2 /media/$USER/RPI-RP2/


