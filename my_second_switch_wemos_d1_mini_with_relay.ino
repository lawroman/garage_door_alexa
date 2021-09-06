#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h> //  https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <ArduinoJson.h> // https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <StreamString.h>


// By Roman Law
// For the "second" device 
// v1.4

// In Linux, update the port permission:   sudo chmod o+rw /dev/ttyUSB0 


ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

#define MyApiKey        "c366863e-6da6-4254-b713-f9288751ccb0"  // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define MySSID          "Law_TP-Link_AD10"                      // TODO: Change to your Wifi network SSID
#define MyWifiPassword  "0011223344"                            // TODO: Change to your Wifi network password

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

int pulseTime = 200; // Controls Relay Active Time in milliseconds, 500 = 0.5sec

                        // TODO:
const int relayPin = 4; //D2 (GPIO 4) 
const int ledPin   = 5; //D1 (GPIO 5) 
                        //D3 (GPIO 0)  

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void setPowerStateOnServer(String deviceId, String value);
void setTargetTemperatureOnServer(String deviceId, String value, String scale);

// deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it here

void turnOn(String deviceId) {
  if (deviceId == "60f2b72d743aaa0466b37174") // Device ID of first device // TODO
  {  
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    
     digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
     delay(pulseTime); // Controls Relay Active Time in milliseconds, 2000 = 2 sec
     digitalWrite(relayPin, LOW);
  } 
  else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);    
  }     
}

void turnOff(String deviceId) {
   if (deviceId == "60f2b72d743aaa0466b37174") // Device ID of first device // TODO
   {  
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);

     digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
     delay(pulseTime); // Controls Relay Active Time in milliseconds, 2000 = 2 sec
     digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW
   }
   else {
     Serial.print("Turn off for unknown device id: ");
     Serial.println(deviceId);    
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Switch or Light device types
        // {"deviceId": xxxx, "action": "setPowerState", value: "ON"} // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html

        // For Light device type
        // Look at the light example in github
          
#if ARDUINOJSON_VERSION_MAJOR == 5
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
        DynamicJsonDocument json(1024);
        deserializeJson(json, (char*) payload);      
#endif        
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if(value == "ON") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }

            digitalWrite(ledPin, LOW);               
            delay(200);
            digitalWrite(ledPin, HIGH);   
        }
        else if (action == "SetTargetTemperature") {
            String deviceId = json ["deviceId"];     
            String action = json ["action"];
            String value = json ["value"];
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300); 
  Serial.println();
  Serial.println();
  Serial.println("v1.4 Device started...");
  
  WiFiMulti.addAP(MySSID, MyWifiPassword);  
  Serial.print("Connecting to Wifi Router: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  int wifiStat = WiFiMulti.run();  
  if(wifiStat == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("WiFi IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Relay PIN eg: https://github.com/wemos/D1_mini_Examples/blob/master/examples/04.Shields/Relay_Shield/Blink/Blink.ino
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);   // Indicator LED active low  
  digitalWrite(ledPin, HIGH);
  delay(1000);
  
  // Display successful start
  if (wifiStat >= 1) {
    for (int i=0; i < wifiStat; i++) {
      digitalWrite(ledPin, LOW);
      delay(800);
      digitalWrite(ledPin, HIGH);
      delay(800);      
    }     
  }
  else {
    // something wrong connecting to wifi
    digitalWrite(ledPin, LOW);
    delay(4000);   
  }

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
}

void loop() {
  webSocket.loop();
  
  if(isConnected) {
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");       

          // blink led
          digitalWrite(ledPin, LOW);               
          delay(1500);
          digitalWrite(ledPin, HIGH);   
          Serial.println("heartbeating @"+ heartbeatTimestamp); 
      }
  }

}

// If you are going to use a push button to on/off the switch manually, use this function to update the status on the server
// so it will reflect on Alexa app.
// eg: setPowerStateOnServer("deviceid", "ON")

// Call ONLY If status changed. DO NOT CALL THIS IN loop() and overload the server. 
void setPowerStateOnServer(String deviceId, String value) {
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  DynamicJsonDocument root(1024);
#endif        
  root["deviceId"] = deviceId;
  root["action"] = "setPowerState";
  root["value"] = value;
  StreamString databuf;
#if ARDUINOJSON_VERSION_MAJOR == 5
  root.printTo(databuf);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  serializeJson(root, databuf);
#endif  
  
  webSocket.sendTXT(databuf);
}

//eg: setPowerStateOnServer("deviceid", "CELSIUS", "25.0")

// Call ONLY If status changed. DO NOT CALL THIS IN loop() and overload the server. 
void setTargetTemperatureOnServer(String deviceId, String value, String scale) {
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  DynamicJsonDocument root(1024);
#endif        
  root["action"] = "SetTargetTemperature";
  root["deviceId"] = deviceId;

#if ARDUINOJSON_VERSION_MAJOR == 5
  JsonObject& valueObj = root.createNestedObject("value");
  JsonObject& targetSetpoint = valueObj.createNestedObject("targetSetpoint");
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  JsonObject valueObj = root.createNestedObject("value");
  JsonObject targetSetpoint = valueObj.createNestedObject("targetSetpoint");
#endif  
  targetSetpoint["value"] = value;
  targetSetpoint["scale"] = scale;
   
  StreamString databuf;
#if ARDUINOJSON_VERSION_MAJOR == 5
  root.printTo(databuf);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  serializeJson(root, databuf);
#endif  
  
  webSocket.sendTXT(databuf);
}
