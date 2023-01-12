#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
//#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <stdio.h>
#include <stdlib.h>

#include "arduino_secrets.h"

#define STRIP_COUNT 5
#define PIECE_COUNT 9
#define MAX_POWER (10000 * 255 / 20)

// Define the led strips attached to the device
Adafruit_NeoPixel strips[STRIP_COUNT] = 
{ 
  Adafruit_NeoPixel(150, 2, NEO_GRB + NEO_KHZ800), // White
  Adafruit_NeoPixel(150, 3, NEO_GRB + NEO_KHZ800), // Yellow 
  Adafruit_NeoPixel(47, 4, NEO_GRB + NEO_KHZ800),  // Red
  Adafruit_NeoPixel(47, 5, NEO_GRB + NEO_KHZ800),  // Blue
  Adafruit_NeoPixel(47, 7, NEO_GRB + NEO_KHZ800)   // Green
};

class StripPiece {
  public:
  StripPiece(int strip, int firstLed, int length) {
    _strip = strip;
    _firstLed = firstLed;
    _length = length;
    _pixels = (uint32_t *)malloc(length * 4);
    for (int k = 0; k < length; k++) {
      _pixels[k] = 0;
    }
  }

  ~StripPiece() { 
    free(_pixels);
    _pixels = NULL;   
  }

  void load_from_mqtt(MqttClient &client) {
    // Fill pixels from mqtt message    
    char buf[9];
    for (int k = 0; k < _length; k++) {
      if (!client.available()) {
        break;
      }

      client.read((unsigned char *)buf, 8);
      buf[8] = NULL;
      uint32_t color = strtoul(buf, NULL, 16);
      _pixels[k] = color;

      while (client.available()) {
        int c = client.read();
        if (c != '\r' && c != '\n' && c != ' ') break;
      }
    }

    // Read rest of message
    while (client.available()) {
      client.read();
    }
  }

  void show_on_strip(Adafruit_NeoPixel *strips, byte brightness, bool isLedsOn) {
    for (int k = _firstLed; k < _firstLed + _length; k++) {
      if (isLedsOn) {
        uint32_t c = _pixels[k - _firstLed];
        uint8_t r = (uint8_t)(c >> 16), g = (uint8_t)(c >> 8), b = (uint8_t)c;      
        r = (r * brightness) / 255;
        g = (g * brightness) / 255;
        b = (b * brightness) / 255;
        strips[_strip].setPixelColor(k, ((uint32_t)r << 16) + ((uint32_t)g << 8) + (uint32_t)b);
      } else {
        strips[_strip].setPixelColor(k, 0);
      }
    }
  }

  private:
  int _strip;
  int _firstLed;
  int _length;
  uint32_t *_pixels;
};

// Define the pieces used to control the strips
StripPiece pieces[PIECE_COUNT] =
{
  StripPiece(0, 0, 46),
  StripPiece(0, 48, 46),
  StripPiece(0, 96, 46),
  StripPiece(1, 0, 46),
  StripPiece(1, 48, 46),
  StripPiece(1, 96, 46),
  StripPiece(2, 0, 46),
  StripPiece(3, 0, 46),
  StripPiece(4, 0, 46)
};

// state
bool isLedsOn = true;
byte brightness = 255;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0;  // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
//WiFiServer server(80);

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "openhabian";
int        port     = 1883;
const char bookshelf_config_topic[]  = "bookshelf_config";

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Initialize NeoPixels
  for (int k = 0; k < STRIP_COUNT; k++) {
    strips[k].begin();
    strips[k].clear();
    strips[k].show();    
  }
    
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  delay(500); // Wait for serial to be open  

  // Connect to wifi
  connect_wifi(); 

  // Connect to mqtt broker
  connect_mqtt(); 

  // start the WiFi OTA library with internal (flash) based storage
  //ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);  
}

void loop() {
  // Check wifi status
  uint8_t status = WiFi.status();
  if (status != WL_CONNECTED) {
    show_status(status, 0x900000);
    Serial.print(status);
    Serial.println();

    connect_wifi();
  }

  // Check connection to mqtt broker
  if (!mqttClient.connected()) {
    show_status(0xFF, 0x008000);    
    Serial.println("MQTT client disconnected");

    connect_mqtt();
  }

  // check for WiFi OTA updates
  //ArduinoOTA.poll();

  // call poll() regularly to allow the library to receive MQTT messages and
  // send MQTT keep alive which avoids being disconnected by the broker
  mqttClient.poll();
}

void connect_mqtt() {
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    show_status(mqttClient.connectError(), 0x008000);

    return;
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  show_status(status, 0x808080);

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  // subscribe to topics
  Serial.print("Subscribing to topics");
  mqttClient.subscribe("piece/0/colors");
  mqttClient.subscribe("piece/1/colors");
  mqttClient.subscribe("piece/2/colors");
  mqttClient.subscribe("piece/3/colors");
  mqttClient.subscribe("piece/4/colors");
  mqttClient.subscribe("piece/5/colors");
  mqttClient.subscribe("piece/6/colors");
  mqttClient.subscribe("piece/7/colors");
  mqttClient.subscribe("piece/8/colors");
  mqttClient.subscribe("allstrips/commands");
}

void connect_wifi() {
  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.noLowPowerMode();
  while (true) {
    show_status(0xFF, 0x000080);
    WiFi.disconnect();    
    WiFi.end();
    int status = WiFi.begin(ssid, pass);
    if (status == WL_CONNECTED) break;

    show_status(status, 0x800000);
    Serial.print(status);
    Serial.println();
    delay(5000);
  }

  Serial.println("You're connected to the network");
  Serial.println();

  show_status(status, 0x808000);
}

// Show an 8 bit status on the strip, one bit per led
void show_status(byte status, uint32_t color) {
  for (int k = 0; k < 8; k++) {
    if ((status >> k) & 0x1) {
      strips[0].setPixelColor(k, color);
    } else {
      strips[0].setPixelColor(k, 0x808080);
    }
  }

  strips[0].show();
}

void onMqttMessage(int messageSize) {
  // we received a message, print out the topic and contents
  String topic = mqttClient.messageTopic();
  Serial.println("Received a message with topic '");
  Serial.print(topic);
  Serial.print("', length ");
  Serial.print(messageSize);  
  Serial.println("");

  if (topic == "allstrips/commands" && messageSize < 10) {
    if (messageSize < 10) {
      char *buffer = (char *)malloc(messageSize + 1);
      mqttClient.read((byte *)buffer, messageSize);
      buffer[messageSize] = NULL;
      String message = String(buffer, messageSize);
      free(buffer);

      handleAllStripsCommand(message);
    } else {
      Serial.println("Message too long, ignoring.");
    }

    while (mqttClient.available()) {
      mqttClient.read();
    }    
  } 
  else if (topic.startsWith("piece/") && topic.endsWith("/colors")) {
    int piece = topic[6] - '0';
    if (piece >= 0 && piece <= PIECE_COUNT) {
      pieces[piece].load_from_mqtt(mqttClient);
      pieces[piece].show_on_strip(strips, brightness, isLedsOn);
      show();
      //printNeoPixels();
    }
  }
}

void handleAllStripsCommand(String &message) {    
  if (message == "ON") {
    Serial.println("Received ON message");
    isLedsOn = true;
    if (brightness == 0) {
      brightness = 255;
    }
  }
  else if (message == "OFF") {
    Serial.println("Received OFF message");
    isLedsOn = false;
  }
  else if (message == "TOGGLE") {
    Serial.println("Received TOGGLE message");
    isLedsOn = !isLedsOn;
    if (isLedsOn && brightness == 0) {
      brightness = 255;
    }
  }
  else if (message == "INCREASE") {
    Serial.println("Received INCREASE message");
    isLedsOn = true;
    int newBrightness = (int)brightness + 16;
    if (newBrightness > 255) {
      brightness = 255;
    } else {
      brightness = newBrightness;
    }
  }
  else if (message == "DECREASE") {
    Serial.println("Received DECREASE message");
    int newBrightness = (int)brightness - 16;
    if (newBrightness < 0) {
      brightness = 0;
    } else {
      brightness = newBrightness;
    }    
  }

  char buf[255];
  sprintf(buf, "Brightness: %d, isLedsOn: %d", brightness, isLedsOn);
  Serial.println(buf);

  mqttClient.beginMessage("allstrips/state");
  mqttClient.println(buf);
  mqttClient.endMessage();

  // Update strips with new brightness values.
  for (int k = 0; k < PIECE_COUNT; k++) {
    pieces[k].show_on_strip(strips, brightness, isLedsOn);
  }

  show();
}


void flash(unsigned long wait) {
  if ((millis() % wait) > (wait / 2)) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  delay(1);
}

void printNeoPixels() {
  for (int strip = 0; strip < STRIP_COUNT; strip++) {
    for (int k = 0; k < strips[strip].numPixels(); k++) {
      uint32_t color = strips[strip].getPixelColor(k);
      char buf[9];
      sprintf(buf, "%08x", color);
      Serial.println(buf);
    }
  }
}

boolean show() {
  // Compute total light power requirements
  long total = 0;
  for (int k = 0; k < STRIP_COUNT; k++) {
    for (int i = 0; i < strips[k].numPixels(); i++) {
      uint32_t color = strips[k].getPixelColor(i);
      total += (color & 0xFF);
      total += ((color & 0xFF00) >> 8); 
      total += ((color & 0xFF0000) >> 16);
    }
  }
  
  if (total < MAX_POWER) {
    for (int k = 0; k < STRIP_COUNT; k++) {
      strips[k].show();
    }
    
    return true;
  }
  
  return false;
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


// void loop() {
//   status = WiFi.status();
//   while (status != WL_CONNECTED) {
//     digitalWrite(LED_BUILTIN, HIGH);
//     Serial.print("Attempting to connect to SSID: ");
//     Serial.println(ssid);
//     // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
//     status = WiFi.begin(ssid, pass);

//     // wait 10 seconds for connection:
//     delay(10000);
    
//     if (status == WL_CONNECTED) {
//       // you're connected now, so print out the status:
//       printWifiStatus();

//       // start the WiFi OTA library with internal (flash) based storage
//       ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);
//     }
//   }
  
//   WiFiClient client = server.available();
//   unsigned long flashInterval = 1500;
//   unsigned long fastFlashEnd = 0;
//   if (client) {
//     Serial.println("new client");
//     flashInterval = 100;
//     fastFlashEnd = millis() + 1000;
//     handleClientRequest(client);
    
//     // give the web browser time to receive the data
//     delay(1);
  
//     // close the connection:
//     client.stop();
//     Serial.println("client disconnected");
//   }
  
//   if (millis() > fastFlashEnd) {
//     flashInterval = 1500;
//   }
  
//   flash(flashInterval);
// }


// void handleClientRequest(WiFiClient client) {
//   String currentLine;
//   String request;
//   int linesRead = 0;
//   while (client.connected()) {
//     if (client.available()) {
//       char c = client.read();
//       Serial.write(c);
      
//       // if you've gotten to the end of the line (received a newline
//       // character) and the line is blank, the http request has ended,
//       // so you can send a reply
//       if (c == '\n' && currentLine.length() == 0) {
//         routeRequest(request, client);
//         break;
//       }
//       if (c == '\n') {
//         linesRead++;
//         if (linesRead == 1) {
//           request = currentLine;
//         }
        
//         currentLine = "";
//       } else if (c != '\r') {
//         currentLine += c;
//       }
//     }
//   }
// }

// void routeRequest(String request, WiFiClient client) {
//   Serial.println("Routing request " + request);
//   if (request == "GET /api/sensors HTTP/1.1") {
//     getSensorStatus(client);
//     return;
//   }
  
//   if (request.startsWith("GET /api/neopixels/")) {
//     int strip = request[19] - '0';
//     if (strip >= 0 && strip < STRIP_COUNT) {
//       getNeoPixels(strip, client);
//       return;
//     }
//   }
  
//   if (request.startsWith("PUT /api/neopixels/")) {
//     int strip = request[19] - '0';
//     if (strip >= 0 && strip < STRIP_COUNT) {
//       putNeoPixels(strip, client);
//       return;
//     }
//   }
  
//   if (request.startsWith("OPTIONS /api/neopixels/")) {
//     int strip = request[23] - '0';
//     if (strip >= 0 && strip < STRIP_COUNT) {
//       corsNeoPixels(strip, client);
//       return;
//     }
//   }
  
//   notFound(client);
// }

// void corsNeoPixels(int strip, WiFiClient client) {
//   client.println("HTTP/1.1 204 No Content");
//   client.println("Connection: close");
//   client.println("Access-Control-Allow-Origin: *");
//   client.println("Access-Control-Allow-Methods: GET, PUT");
//   client.println("Access-Control-Max-Age: 172800"); // How long the results can be cached in seconds
//   client.println();
// }

// void putNeoPixels(int strip, WiFiClient client) {
//   char buf[9];
//   for (int k = 0; k < strips[strip].numPixels(); k++) {
//     client.read((unsigned char *)buf, 8);
//     buf[8] = NULL;
//     long color = strtol(buf, NULL, 16);
//     strips[strip].setPixelColor(k, color);
    
//     if (client.read() != '\r') break;
//     if (client.read() != '\n') break;
//   }
  
//   if (show()) {
//     client.println("HTTP/1.1 200 OK");
//     client.println("Connection: close");
//     client.println("Access-Control-Allow-Origin: *");
//     client.println();
//   } else {
//     client.println("HTTP/1.1 403 Invalid");
//     client.println("Connection: close");
//     client.println();
//   }
// }

// void notFound(WiFiClient client) {
//   // send a standard http response header
//   client.println("HTTP/1.1 404 Not found");
//   client.println("Content-Type: text/html");
//   client.println("Connection: close");  // the connection will be closed after completion of the response
//   client.println();
//   client.println("<!DOCTYPE HTML>");
//   client.println("<html>");
//   client.println("<body>The requested resource was not found.</body>");
//   client.println("</html>");
// }

// void getSensorStatus(WiFiClient client) {
//   // send a standard http response header
//   client.println("HTTP/1.1 200 OK");
//   client.println("Content-Type: text/html");
//   client.println("Connection: close");  // the connection will be closed after completion of the response
//   client.println("Refresh: 5");  // refresh the page automatically every 5 sec
//   client.println();
//   client.println("<!DOCTYPE HTML>");
//   client.println("<html>");
//   // output the value of each analog input pin
//   for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
//     int sensorReading = analogRead(analogChannel);
//     client.print("analog input ");
//     client.print(analogChannel);
//     client.print(" is ");
//     client.print(sensorReading);
//     client.println("<br />");
//   }
//   client.println("</html>");
// }

