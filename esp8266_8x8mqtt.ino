/***************************************************
  8x8 LED matrix with MQTT for ESP8266

  Based on ...

  Adafruit MQTT Library ESP8266 Adafruit IO SSL/TLS example

  Must use the latest version of ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  SSL/TLS additions by Todd Treece for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include <Wire.h>
#include "Timer.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

// WiFi ap and aio creditials are in secret.h
#include "secret.h"
#include "icons.h"

/************ Global State (you don't need to change this!) ******************/

// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// io.adafruit.com SHA1 fingerprint
const char* fingerprint = "26 96 1C 2A 51 07 FD 15 80 96 93 AE F7 32 CE B9 0D 01 55 C4";

/****************************** Feeds ***************************************/

// Setup a feed called 'test' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish pub_debug = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/matrix_debug");
Adafruit_MQTT_Subscribe sub_matrix = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/matrix_command");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();
void verifyFingerprint();

uint8_t delay_ms = 100;
uint8_t nrepeat = 3;

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();


void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(F("MQTT matrix"));

  Wire.pins(4,5); // ESP8266 default I2C pins: SCL 4, SDA 5
  matrix.begin(0x70);
  matrix.setRotation(0);
  matrix.setBrightness(0); //0 to 15
  matrix.blinkRate(3);
  matrix.clear();
  matrix.writeDisplay();

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  delay(1000);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  delay(2000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // check the fingerprint of io.adafruit.com's SSL cert
  verifyFingerprint();

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&sub_matrix);

  matrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON);
  matrix.writeDisplay();
}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &sub_matrix) {
      Serial.print(F("Got: "));
      Serial.println((char *)sub_matrix.lastread);
      Serial.print(F("Data length: "));
      Serial.println(sub_matrix.datalen);
      
      if (sub_matrix.datalen > 2){
        // received at least two characters
        uint8_t cmd = getByte((char *)sub_matrix.lastread, 0);
        Serial.print(F("CMD: "));
        Serial.println(cmd, HEX);
        if (cmd == 0x00){
          // Test commands
          Serial.print(F("Test command: "));
          uint8_t sub_cmd = getByte((char *)sub_matrix.lastread, 2);
          Serial.println(sub_cmd, HEX);
          if (sub_cmd == 0xFF){
            // back to default current draw
            Serial.println(F("Default standby display"));
            matrix.clear();
            matrix.setBrightness(0); // 0 dimm to 15 brightest
            matrix.blinkRate(3); //0: off, 1: 2Hz 2: 1Hz 3: 0.5Hz
            matrix.drawBitmap(0, 0, dot_bmp, 8, 8, LED_ON); // smile_bmp, neutral_bmp, frown_bmp
            matrix.writeDisplay();
          }
          else if (sub_cmd == 0xFE){
            // draw max current
            Serial.println(F("Draw max current"));
            matrix.clear();
            matrix.setBrightness(15); // 0 dimm to 15 brightest
            matrix.blinkRate(0); //0: off, 1: 2Hz 2: 1Hz 3: 0.5Hz
            matrix.drawBitmap(0, 0, allon_bmp, 8, 8, LED_ON); // smile_bmp, neutral_bmp, frown_bmp
            matrix.writeDisplay();
          }
          else if (sub_cmd == 0xFD){
            // Good for current draw test
            Serial.println(F("Default current draw"));
            matrix.clear();
            matrix.setBrightness(0); // 0 dimm to 15 brightest
            matrix.blinkRate(3); //0: off, 1: 2Hz 2: 1Hz 3: 0.5Hz
            matrix.drawBitmap(0, 0, rain_bmp, 8, 8, LED_ON); // smile_bmp, neutral_bmp, frown_bmp
            matrix.writeDisplay();
          }
          else if (sub_cmd == 0xFC){
            // draw a character
            Serial.println(F("Draw A character"));
            unsigned char c = 'A';
            matrix.clear();
            matrix.drawChar(0, 0, c, LED_ON, LED_OFF, 1);
            //matrix.print(msg);
            matrix.writeDisplay();
          }
          else if (sub_cmd == 0x00){
            // smiley
            Serial.println(F("Put smiley face on"));
            matrix.clear();
            matrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON); // smile_bmp, neutral_bmp, frown_bmp
            matrix.writeDisplay();
          }
          else if (sub_cmd == 0x01){
            Serial.println(F("Put neutral face on"));
            matrix.clear();
            matrix.drawBitmap(0, 0, neutral_bmp, 8, 8, LED_ON); // smile_bmp, neutral_bmp, frown_bmp
            matrix.writeDisplay();
          }
          else if (sub_cmd == 0x02){
            Serial.println(F("Put frown face on"));
            matrix.clear();
            matrix.drawBitmap(0, 0, frown_bmp, 8, 8, LED_ON); // smile_bmp, neutral_bmp, frown_bmp
            matrix.writeDisplay();
          }
        }
        else if (cmd == 0x01){
          // Arbitrary icon display command
          // send sunny icon
          // mosquitto_pub -t DEVICEID/feeds/matrix_command -m 0108221C551C22080000
          // 01 Arbitrary icon display command
          // icon data
          // 00 Duration to display the icon in seconds. 0 to keep displaying
          Serial.println(F("Arbitrary icon display command"));
          if (sub_matrix.datalen == 20){
            uint8_t icon[8];
            for(int i=0; i<8; i++){
              icon[i] = getByte((char *)sub_matrix.lastread, i*2+2);
              //icon[i] = getVal(sub_matrix.lastread[i*2+2+1]) + (getVal(sub_matrix.lastread[i*2+2]) << 4);
            }
            uint8_t duration = getByte((char *)sub_matrix.lastread, 18);
            //TODO: set duration
            matrix.clear();
            matrix.drawBitmap(0, 0, icon, 8, 8, LED_ON);
            matrix.writeDisplay();
          }
          else{
            Serial.println(F("Wrong data length"));
          }
        }
        else if (cmd == 0x02){
          // Text scrolling command
          // send scrolling text
          // mosquitto_pub -t DEVICEID/feeds/matrix_command -m "02Max 17 characters"
          // 02 Text scrolling command
          // The rest is message character array (not in hex string!)
          Serial.println(F("Text scrolling command"));
          if (sub_matrix.datalen > 2){
            matrix.blinkRate(0); //0: off, 1: 2Hz 2: 1Hz 3: 0.5Hz
            Serial.println((char *)sub_matrix.lastread+2);
            scrollMessage3((char *)sub_matrix.lastread+2, sub_matrix.datalen-2);  // discard first two bytes
            //TODO: Display default standby
          }
          else{
            Serial.println(F("Wrong data length"));
          }
        }
        else if (cmd == 0x03){
          // Text scrolling control command
          // set to mid speed and 10 times repeat
          // mosquitto_pub -t DEVICEID/feeds/matrix_command -m "030F0A"
          // 03 Text scrolling control command
          // 0F Scrolling speed; 00 (slow) to FF (fast)
          // 0A No. of repeat; 00 is infinite
          Serial.println(F("Text scrolling control command"));
          if (sub_matrix.datalen == 6){
            uint8_t delay_0255 = getByte((char *)sub_matrix.lastread, 2);
            delay_ms = map(delay_0255, 0, 255, 0, 1000); // map 0-255 value to 0 to 1000 millisec delays
            nrepeat = getByte((char *)sub_matrix.lastread, 4);
            Serial.print(F("Delay [ms]: "));
            Serial.println(delay_ms);
            Serial.print(F("Repeat: "));
            Serial.println(nrepeat);
          }
          else{
            Serial.println(F("Wrong data length"));
          }
        }
        else if (cmd == 0xFF){
          // 8x8 LED display settings command
          Serial.print(F("Settings command: "));
          uint8_t sub_cmd = getByte((char *)sub_matrix.lastread, 2);
          Serial.println(sub_cmd, HEX);
          if (sub_cmd == 0x00){
            // set brightness
            //matrix.setBrightness(5); // 0 dimm to 15 brightest
            uint8_t b = getByte((char *)sub_matrix.lastread, 4);
            if (b>15) b = 15;
            matrix.setBrightness(b);
            matrix.writeDisplay();
            Serial.print(F("Set brightness: "));
            Serial.println(b);
          }
          else if (sub_cmd == 0x01){
            // set Rotation
            //matrix.setRotation(3); // 0, 1, 2, 3
            uint8_t b = getByte((char *)sub_matrix.lastread, 4);
            if (b>3) b = 3;
            matrix.clear();
            matrix.setRotation(b);
            //matrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON); // smile_bmp, neutral_bmp, frown_bmp
            matrix.writeDisplay();
            Serial.print("Set rotation: ");
            Serial.println(b);
          }
          else if (sub_cmd == 0x02){
            // set Blink Rate
            //matrix.blinkRate(3); //0: off, 1: 2Hz 2: 1Hz 3: 0.5Hz
            uint8_t b = getByte((char *)sub_matrix.lastread, 4);
            if (b>3) b = 3;
            matrix.blinkRate(b);
            matrix.writeDisplay();
            Serial.print("Set blink rate: ");
            Serial.println(b);
          }
        }
        else {
          Serial.println(F("No CMD found"));
        }
      }
    }
  }
}

void publishMessage(String message){
  //String message = "Hi";
  int len = message.length();
  char msg[len];
  message.toCharArray(msg, len);
  Serial.print(F("Publishing: "));
  Serial.println(message);
  if (! pub_debug.publish(msg))
    Serial.println(F("Failed to publish"));
   else{
    Serial.println(F("OK"));
   }
}

//void scroll_message(int delay_ms){
//  // Hello YouTube! on ESP-12 and AdaFruit I2C LED Matrix
//  // Hari Wiguna, 2015
//  // Draw the message with increasing negative offset to "scroll" it
//  // delay_ms: 300 to 30 ms slow to fast scroll delay time
//  for (int i = 0; i < msgLength; i++)
//    matrix.drawChar(i*6 - offset, 0, msg[i], 1, 0, 1);
//
//  matrix.writeDisplay();  // Actually update the display
//
//  if (offset++ >= msgLength*6) offset = 0;
//
//  delay(delay_ms);
//}
//
//void scrollMessage2(){
//  // from Adafruit matrix8x8 example
//  matrix.setTextSize(1);
//  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
//  matrix.setTextColor(LED_ON);
//  for (int8_t x=0; x>=-36; x--) {
//    matrix.clear();
//    matrix.setCursor(x,0);
//    matrix.print("Hello");
//    matrix.writeDisplay();
//    delay(100);
//  }
//}

void scrollMessage3(char *msg, int msgLength){
  // from Adafruit matrix8x8 example
  // modified to take char array and delay argument
  matrix.setTextSize(1);
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  //TODO: make this function responsive to new MQTT sub message
  for (int i=0; i<nrepeat; i++){
    for (int16_t x=0; x>=-6*msgLength-6; x--) {
      matrix.clear();
      matrix.setCursor(x,0);
      matrix.print(msg);
      matrix.writeDisplay();
      delay(delay_ms);
    }
  }
}

void verifyFingerprint() {

  const char* host = AIO_SERVER;

  Serial.print("Connecting to ");
  Serial.println(host);

  if (! client.connect(host, AIO_SERVERPORT)) {
    Serial.println("Connection failed. Halting execution.");
    while(1);
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("Connection secure.");
  } else {
    Serial.println("Connection insecure! Halting execution.");
    while(1);
  }

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }

  Serial.println("MQTT Connected!");
}

// hexString to hex value
// http://www.theskyway.net/en/converting-hex-as-a-string-to-actual-hex-values/
uint8_t getVal(char c)
{
   if(c >= '0' && c <= '9')
     return (uint8_t)(c - '0');
   else
     return (uint8_t)(c-'A'+10);
}

uint8_t getByte(char * strhex, uint8_t pos){
  return getVal(strhex[pos+1]) + (getVal(strhex[pos]) << 4);
}
