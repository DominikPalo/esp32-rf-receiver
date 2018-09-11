# ESP32 RF receiver
An ESP32 (ESP-IDF SDK) application to receive signals using 315/433 MHz RF modules. Most of the code is ported from the [Arduino RC-Switch library](https://github.com/sui77/rc-switch) - rewritten from C++ to C. Some functions which are not contained in ESP-IDF SDK are ported from the [Arduino core for ESP32](https://github.com/espressif/arduino-esp32) project.

## Non-blocking loop
Mac Wyznawca make some changes. Non-blocking loop with Queue and ESP-SDK native function esp_timer_get_time() for millisecond.

## Installation
1. Prepare your ESP-IDF environment (http://esp-idf.readthedocs.io/en/latest/get-started/)
2. Clone this repository `git clone https://github.com/DominikPalo/esp32-rf-receiver.git`
3. Connect ESP32 board to PC and check under what serial port the board is visible
4. Configure the serial port which will be used to flash this application (port from the previous step):
    * Start the project configuration utility: `make menuconfig`
    * In the menu, navigate to `Serial flasher config` > `Default serial port` to configure the serial port, where project will be loaded to. Confirm selection by pressing enter, save configuration by selecting `< Save >` and then exit application by selecting `< Exit >`.
5. Build and flash the application. Run: `make flash` - this will compile the application and all the ESP-IDF components, generate bootloader, partition table, and application binaries, and flash these binaries to your ESP32 board.

## Hardware
You can use any 315/433 Mhz 3.3V compatible RF receiver, but from my experience I highly recommend an [RXB6 Superheterodyne module](http://www.jmrth.com/en/images/proimages/RXB6_en_v3.pdf) (also known as 3400RF) - it has very good sensitivity/range and costs only about $1.5 on eBay.

See: [**How to connect an RF module to ESP32**](wiring.png?raw=true) 
(DATA pin can be changed by setting DATA_PIN in the `esp32_rf_receiver.c` file)

### Antenna
To get a full range of the RF receiver, you have to attach an external antenna to the ANT pin. A simple wire is sufficient - the length of the wire should be 22.6 cm for a 315 MHz module or 17.2 cm for a 433 MHz module.

## Usage
Just connect with your terminal (PuTTY, CoolTerm, etc.) to the ESP32 serial port (baudrate 115200) and start emitting RF signals. All received signals will be outputed. You can choose between two types of output by setting the value of the `ADVANCED_OUTPUT`:
* Simple output (`#define ADVANCED_OUTPUT 0`)
* Advanced output (`#define ADVANCED_OUTPUT 1`)
