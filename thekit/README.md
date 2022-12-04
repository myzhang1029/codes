# The Kit
The Kit is a collection of device/sensor management and remote control
solutions that I made at Pearson College UWC.

## First Gen
The first generation of The Kit is an Arduino program (ArduinoTheKit)
and a Rust daemon (thekitd) meant to be run on another Linux machine,
connected to the Arduino via serial.
Basically everything else is based on this interface.

## Second Gen
The second generation consists of a Raspberry Pi daemon (thekitd2.py)
and a Raspberry Pi Pico (thekit2_pico) connected to WiFi via a ESP8266
(thekit_receiver).
There is a slightly more versatile version, thekitd2.5.py.

## Third Gen
To minimize power consumption and CPU usage, the third generation is
a Linux Kernel Module (thekit3_pwm.c) for Raspberry Pi and a corresponding
`inetd`-style HTTP daemon (thekitd3.c).