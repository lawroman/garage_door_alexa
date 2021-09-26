/* 
   Based on and modified from SinricPro libarary (https://github.com/sinricpro/)
   to support other boards such as  SAMD21, SAMD51, Adafruit's nRF52 boards, etc.
 
   Use WeMos D1 Mini board from Tools -> Boards

   Relay shield: https://docs.wemos.cc/en/latest/d1_mini_shiled/relay.html

   If you encounter any issues:
   - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
   - ensure all dependent libraries are installed
     - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
     - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
   - open serial monitor and check whats happening
   - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
   - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
*/

// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT      Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif

#include <Arduino.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#endif

#ifdef ESP32
  #include <WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProGarageDoor.h"
#include <StreamString.h>

/*
 By Roman Law
 For the "second" device 

 Release Notes: upgrading to switch to sinric pro server

 In Linux, update the port permission:   sudo chmod o+rw /dev/ttyUSB0 

 Uses Arduino IDE to modify/compile/upload to device
*/

#define SOFTWARE_VERSION  "v2.1.9e"              // TODO: the version of this software
#define WIFI_SSID         "Law_TP-Link_AD10"  // TODO: Change to your Wifi network SSID
#define WIFI_PASS         "0011223344"        // TODO: Change to your Wifi network password
                                                         
#define APP_KEY     "2e144658-c03e-4fa4-a78c-3376c883b345"  // TODO: Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET  "8df82acb-5d3c-48e9-8dcc-b4368986f427-43db8456-a940-4e51-b055-90c2c920c2b6"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define GARAGEDOOR_ID     "614f46c86f1af10712a38b8a"    // Should look like "5dc1564130xxxxxxxxxxxxxx"
#define BAUD_RATE         115200                // Change baudrate to your need

#define LED_PIN           D3                  // * Light up external LED when D1 is HIGH
#define RELAY_PIN         D4                  // * Relay Shield transistor closes relay when D2 is HIGH

int pulseTime = 200;      // Controls Relay Active Time in milliseconds, 500 = 0.5sec
bool myPowerState = false;

/* bool onDoorState(String deviceId, bool &state)

   Callback for onDoorState request
   parameters
    String deviceId (r)
      contains deviceId (useful if this callback used by multiple devices)
    bool &state (r/w)
      contains the requested state (true:on / false:off)
      must return the new state

   return
    true if request should be marked as handled correctly / false if not
*/
bool onDoorState(const String& deviceId, bool &doorState) {
  Serial.printf("\nGaragedoor is %s now.\r\n", doorState?"closed":"open");
  
  // set the relay on wemos
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, HIGH);
  delay(pulseTime);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);
  return true;
}

// setup function for WiFi connection
void setupWiFi() 
{
  Serial.printf("\r\n[Wifi]: Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.printf(".");
    delay(250); 
  }

  digitalWrite(LED_PIN, HIGH);
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

// setup function for SinricPro
void setupSinricPro() 
{
  Serial.printf("Trying to connnect to SinricPro...\r\n");
  
  SinricProGarageDoor &myGarageDoor = SinricPro[GARAGEDOOR_ID];
  myGarageDoor.onDoorState(onDoorState);

  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup() 
{ 
  pinMode(RELAY_PIN, OUTPUT); // define Relay GPIO as output
  pinMode(LED_PIN, OUTPUT);   // Indicator LED active low  
  digitalWrite(LED_PIN, HIGH);
  
  Serial.begin(BAUD_RATE); 
  while (!Serial);
  delay(2500);
  
  Serial.println("\nDevice firmware " + String(SOFTWARE_VERSION));
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("\nStarting WeMosD1_mini_relay_shield on " + String(ARDUINO_BOARD));
  Serial.println("Version : " + String(SINRICPRO_VERSION_STR));
  
  setupWiFi();
  setupSinricPro();
}

void loop() 
{
  SinricPro.handle();
}
