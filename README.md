# iDispenser Embedded Application Version 1.0
Version 1.0 is the first protoype using servo motor for mechanical spraying action.

## Hardware
- LPC1758 ARM Cortex M3 Embedded Development Board aka SJOne Board
- Adafruit Bluefruit BLE (Bluetooth Module)
- ESP8266 ESP-01 (WiFi Module)
- Servo Motor HiTec HS-311

## Overview
The application is developed using C, C++ and FreeRTOS. There are three main tasks running in FreeRTOS namely, WiFi, Bluetooth BLE and 
dc motor for mechanical spraying. 
The WiFi task connects to the wireless network assigned and, establishes TCP connection to the cloud server for sending current 
temperature data. Depending on the response received from the server, the application either initiates dispenser action or continues 
sending data. Also, the application allows Bluetooth connection via iOS mobile app to test the dispenser spray manually. 

## Author
Prakash Kurup

## Reference
Preetpal Kang, SJ One Board, SocialLedge.com
