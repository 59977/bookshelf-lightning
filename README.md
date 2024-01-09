## Recommended setup:
Raspberry PI or equivalent running:
* Openhabian
* Mosquito mqtt broker (Can be installed using OpenHabian config tool: `sudo openhabian-config`)

## Install bookshelf app:
Copy application files to /usr/share/node/bookshelf, i.e. these files:
* bookshelf_config.js  
* bookshelf.js       
* configure.html  
* index.php                  
* react-input-slider-browser.js
* bookshelf.css        
* bookshelf.service  
* index.html      
* react-colorful-browser.js

Copy bookshelf.service to /etc/systemd/system

MQTT Broker expected on: openhabian:1883

## Install / update device

Use Arduino IDE, tested on version 2.0.3. Follow instructions on https://www.arduino.cc/en/software.

Install the board `esp8266 by ESP8266 Community` (tested on version 3.1.0)

Install libraries:
* ArduinoMqttClient (tested on version 0.1.6)
* Adafruit Neopixel (tested on version 1.10.7)
* ArduinoOTA (tested on version 1.0.9)

The device can be updated over-the-air (OTA) without the need to connect a physical cable. It is also possible to attach a USB data cable to the device.

### WIFI settings

Should be defined in arduino/bookshelf/arduino_secrets.h:
```
#define SECRET_SSID "<SSID>"
#define SECRET_PASS "<password>"
```

Replace &lt;SSID> and &lt;password> with your network SSID and password.