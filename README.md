# Flexible bookshelf lightning

A system to support having neopixel led strips in a bookshelf (or anywhere) and be able to control each individual led using an app for a customized lighting setup. The app also has on / off and possibility to dim current led setup while still remembering the original setup.

Currently only static light is supported (no animation).

The system consists of:
* ESP8266 device controlling the neopixel strips
* React web application for controlling the color of each led
* MQTT Broker for communication

## Board layout

Standard neopixel setup (use signal amplifier if running long data wires). Multiple strips can be controlled using the app, but that needs to be configured in the sketch. 

A strip can be setup as multiple patches simulating more strips in the app. This is useful if running a strip through a wall in the bookshelf to save on the wiring required.

## Application server
Recommended to use a Raspberry PI or equivalent running:
* Openhabian
* Mosquito mqtt broker (Can be installed using OpenHabian config tool: `sudo openhabian-config`)

If possible call the device openhabian on the network. The web app will then be available on `http://openhabian:3000/configure.html`.

MQTT Broker expected on: `openhabian:1883` (Can be changed in sketch if needed). Authentication not supported right now.

### Install web app
Copy application files from web folder to `/usr/share/node/bookshelf`.

Copy bookshelf.service to `/etc/systemd/system`. This will make the web server start automatically when the device starts. 

To start manually run: `/usr/bin/http-server -p 3000` from `/usr/share/node/bookshelf` folder.

## ESP8266 Device

It is recommended to use an ESP8266 device.

### Upload sketch

Use Arduino IDE, tested on version 2.0.3. Follow instructions on https://www.arduino.cc/en/software.

Install the board `esp8266 by ESP8266 Community` (tested on version 3.1.0)

Install libraries:
* ArduinoMqttClient (tested on version 0.1.6)
* Adafruit Neopixel (tested on version 1.10.7)
* ArduinoOTA (tested on version 1.0.9)

Attach USB data cable and upload using Arduino IDE.

## Update sketch 

Once the sketch is installed on the device it can be updated over-the-air (OTA) without the need to connect a physical cable. It is also possible to attach a USB data cable to the device.

### WIFI settings

Should be defined in arduino/bookshelf/arduino_secrets.h:
```
#define SECRET_SSID "<SSID>"
#define SECRET_PASS "<password>"
```

Replace &lt;SSID> and &lt;password> with your network SSID and password.

_IMPORTANT_: If changing WIFI password remember to update the device with the new password before - otherwise over-the-air updates will be impossible for obvious reasons.

## Optional components

### Control on / off / fade with IKEA controller

This is something I built to be able to turn on / off and dim the bookshelf without having to use an app.

Components:
* IKEA Tradfri controller
* Zigbee radio USB dongle
* ZigbeeMQTT 

Add IKEA controller to ZigbeeMQTT.
Add a script in Openhab that translates the IKEA commands into bookshelf MQTT commands.
