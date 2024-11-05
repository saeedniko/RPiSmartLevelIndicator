# RPi_Smart_Level_Indicator
A smart level indicator system using a Raspberry Pi, OLED display, ultrasonic sensor, LED indicator, and Bluetooth connectivity. This project measures and displays the fill level of a container and wirelessly transmits the data to an Android app.

# Setup and Installation

## 1.Compile the Device Tree Overlay

Use the following command to compile the Device Tree Source (DTS) file into a Device Tree Blob (DTBO):

`dtc -@ -I dts -O dtb -o OverLaysSR04.dtbo OverLaysSR04.dts`

## 2.Apply the Overlay

Load the compiled overlay with the following command:

`sudo dtoverlay OverLaysSR04.dtbo`

## 3.Build the Project

Compile the necessary code by running:

`make`
