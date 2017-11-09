# esp32-rf-receiver
The ESP32 wireless receiver for 315/433MHz modules based on ESP-IDF SDK.

## Installation
1. Prepare your ESP-IDF environment (http://esp-idf.readthedocs.io/en/latest/get-started/)
2. Clone this repository `git clone https://github.com/DominikPalo/esp32-rf-receiver.git`
3. Connect ESP32 board to PC and check under what serial port the board is visible
4. Configure the serial port which be used to flash this application (port the previous step):
    * Start the project configuration utility: `make menuconfig`
    * In the menu, navigate to `Serial flasher config` > `Default serial port` to configure the serial port, where project will be loaded to. Confirm selection by pressing enter, save configuration by selecting `< Save >` and then exit application by selecting `< Exit >`.
5. Build and flash the application. Run: `make flash` - this will compile the application and all the ESP-IDF components, generate bootloader, partition table, and application binaries, and flash these binaries to your ESP32 board.

## Hardware
You can use any 315/433Mhz 3.3V compatible RF receiver, but from my experience I highly recommend an [RXB6 Superheterodyne module](http://www.jmrth.com/en/images/proimages/RXB6_en_v3.pdf) (also known as 3400RF) - it has very good sensitivity/range and costs only about $1.5 on eBay.

### How to connect an RF module to ESP32
```
RF rec.   ESP32
---------------
GND  ---> GND
VCC  ---> 3.3V
DATA ---> GPIO#22 
```
(DATA pin can be changed by setting DATA_PIN in the esp32_rf_receiver.c file)

## Usage
Just connect with your terminal (PuTTY, CoolTerm, etc.) to the ESP32 serial port (baudrate 115200) and start emitting your RF signals. All received signals will be outputed.
