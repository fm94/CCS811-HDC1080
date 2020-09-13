#include "Adafruit_CCS811.h"
#include "ClosedCube_HDC1080.h"
#include <ESP8266WiFi.h>

extern "C" {
  #include "gpio.h"
}

extern "C" {
  #include "user_interface.h"
}

Adafruit_CCS811 ccs;
ClosedCube_HDC1080 hdc1080;

double tmp_hdc = 0;
double humidity = 0;
double eco2 = 400;
double tvoc = 0;

const char *ssid = "3neo_2.4Ghz_D72F";
const char *password = "5Y3p8HnQjC";

const char* apiKey = "TZ9KBKDN8AGH1AUB";
const char* resource = "/update?api_key=";
const char* server = "api.thingspeak.com";

long startTime ;

// Set your Static IP address
IPAddress local_IP(192, 168, 0, 192);
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

void setup() {
  
  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();
  
  Serial.begin(9600);
  delay(10);
  Serial.println("Setup!");

  if ((*resetInfo).reason != 5){
      while(!ccs.begin()){
        Serial.println("Failed to start sensor! Please check your wiring.");
        delay(1000);
      }
      
      Serial.println("set 60 secs");
      ccs.setDriveMode(CCS811_DRIVE_MODE_60SEC);
      delay(100);
      
      startTime = millis();
    
      hdc1080.begin(0x40);
    
      tmp_hdc = hdc1080.readTemperature() - 6;
      humidity = hdc1080.readHumidity();
      ccs.setEnvironmentalData(tmp_hdc+6, humidity);
      delay(100);

      ESP.deepSleep((60-(millis() - startTime)/1000-1)*1e6);
      
  }
  else{
    while(!ccs.begin_deep()){
        Serial.println("Failed to start sensor after deep! Please check your wiring.");
        delay(1000);
      }
      hdc1080.begin(0x40);
    }
}


void makeHTTPRequest(double tmp_hdc, double humidity, double eco2, double tvoc) {

  WiFiClient client;
  int retries = 10;
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
    delay(250);
  }
  Serial.println();
  if(!!!client.connected()) {
     Serial.println("Failed to connect server, going back to sleep");
     ESP.deepSleep(45*1e6);
  }
  
  //Serial.print("Request resource: "); 
  //Serial.println(resource);
  client.print(String("GET ") + resource + apiKey + "&field1=" + tmp_hdc + "&field2=" + humidity + "&field3=" + eco2 + "&field4=" + tvoc +
                  " HTTP/1.1\r\n" +
                  "Host: " + server + "\r\n" + 
                  "Connection: close\r\n\r\n");
                  
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
     Serial.println("No response, going back to sleep");
  }
  if(client.available()){
    //Serial.write(client.read());
    Serial.write("We got an answer!");
  }
  
  //Serial.println("\nclosing connection");
  Serial.println("\nData Published!");
  
  client.stop();
}

void initWifi() {

   Serial.print("Connecting to: "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  
  // WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)
  //if (!WiFi.config(local_IP, gateway, subnet)) {
   // Serial.println("STA Failed to configure");
  //}
  
  

  int timeout = 50 * 4; // 20 seconds
  while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() != WL_CONNECTED) {
     Serial.println("Failed to connect wifi, going back to sleep");
     ESP.deepSleep(45*1e6);
  }

  Serial.print("Address: "); 
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}


void loop() {
  
  while(!ccs.available()){
    Serial.println("Waiting for data");
    delay(500);
    };

  startTime = millis();
  
  if(!ccs.readData()){ 
    eco2 = ccs.geteCO2();
    tvoc = ccs.getTVOC();

    tmp_hdc = hdc1080.readTemperature() - 6;
    humidity = hdc1080.readHumidity();

    initWifi();

    Serial.println(tmp_hdc);
    Serial.println(humidity);
    Serial.println(eco2);
    Serial.println(tvoc);

    ccs.setEnvironmentalData(tmp_hdc+6, humidity);

    if (eco2 != 0){
          makeHTTPRequest(tmp_hdc, humidity, eco2, tvoc);
          Serial.println("fake sending...");
          }
    
  }

  Serial.write("Sleep!");
  delay(1000);
  
  ;
  
  ESP.deepSleep((60-(millis() - startTime)/1000-1)*1e6);
  delay(100);
  Serial.write("Woke up!");
}
