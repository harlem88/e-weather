E-WEATHER
==========

## How to use

### Setup project

* Add EPDiy E-Paper Driver component
    ``` bash
    git clone https://github.com/vroland/epdiy
    mv ./epdiy/src/epd_driver ./components
    rm -rf epdiy
    ```

### Configure the project

Open the project configuration menu (`idf.py menuconfig`).

In the `E-WEATHER WiFi Configuration` menu:

* Set the Wi-Fi configuration.
    * Set `WiFi SSID`.
    * Set `WiFi Password`.

### Build and Flash

Build the project and flash it to the board, then run the monitor tool to view the serial output:
Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.
(To exit the serial monitor, type ``Ctrl-]``.)
