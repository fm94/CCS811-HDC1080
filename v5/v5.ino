///////////////////////////////////////////////////////////////////////////////////////////
// By Fares Meghdouri
// 13.09.2020
// Description: A script tested with ESP8266 (NodeMCU) and the CCS811/HD1080 sensors.
//              The script collects sensor readings and post them to a thingspeak channel.
//              The routine includes also a Deep Sleeep mode to save power.
//////////////////////////////////////////////////////////////////////////////////////////

// Import various libraries
#include "Adafruit_CCS811.h"
#include "ClosedCube_HDC1080.h"
#include <ESP8266WiFi.h>

extern "C" {
  #include "gpio.h"
}

extern "C" {
  #include "user_interface.h"
}

// Define the sensors objects
Adafruit_CCS811 ccs;
ClosedCube_HDC1080 hdc1080;

// Define variables used throught the code
double tmp_hdc = 0;
double humidity = 0;
double eco2 = 400;
double tvoc = 0;

// Put your wifi info here
const char *ssid = "";
const char *password = "";

// Put you channel write key here
const char* apiKey = "";
const char* resource = "/update?api_key=";
const char* server = "api.thingspeak.com";

// Measure the execution time so that the deep sleep of the sensor matches the Deep Sleep of the ESP8266
long startTime ;

// I experimented with this section to use static wifi configuration but it didn't work so far
// Set your network details
IPAddress local_IP(192, 168, 0, 192);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

// start the configuration (this will run after each Deep Sleep)
void setup() {

  // read the reset reason
  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();
  
  Serial.begin(9600);
  delay(10);
  Serial.println("Setup!");

  // If first time staring, then configure sensors
  if ((*resetInfo).reason != 5){
      while(!ccs.begin()){
        Serial.println("Failed to start sensor! Please check your wiring.");
        delay(1000);
      }

      // set the period to 60 seconds
      Serial.println("set 60 secs");
      ccs.setDriveMode(CCS811_DRIVE_MODE_60SEC);
      delay(100);

      // since the sensor already started the 60 seconds, we need to keep track of the next wake-up
      startTime = millis();
    
      hdc1080.begin(0x40);

      // reduce 6 degrees from the actual reading due to the ccs811 heating up the hdc1080 (this is experimental)
      tmp_hdc = hdc1080.readTemperature() - 6;
      humidity = hdc1080.readHumidity();

      // push readings (I'm not sure if we should push them after or before sleep?)
      ccs.setEnvironmentalData(tmp_hdc+6, humidity);
      delay(100);

      // sleep for 60 seconds - the time needed for the configuration above. This way, we wake up
      // imediately before the readings. Sleeping while the readings from the sensors
      // are ready causes wrong readings. Check the tutorial on my website to understand better.
      ESP.deepSleep((60-(millis() - startTime)/1000-1)*1e6);
      
  }
  else{
    // if we woke up from Deep Sleep then only initialise the css811 with the modified version of begin
    // (can be found in my github repo). Initialize also the hdc1080 since the later does not cause
    // any problems.
    while(!ccs.begin_deep()){
        Serial.println("Failed to start sensor after deep! Please check your wiring.");
        delay(1000);
      }
      hdc1080.begin(0x40);
    }
}


// A function that takes the four input variables and post them to Thingspeak
// Should be self-explanatory, you can also put yours here.
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

// A function to initialize the wifi module and Deep Sleep if it fails.
// I tried to configure the IP statistically to speed-up the connection establishement
// but it did not work so I droped the idea. The wifi connection with the current code takes between 2 to 5 seconds.
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
     // sleep untill the next sensor reading.
     ESP.deepSleep(45*1e6);
  }

  Serial.print("Address: "); 
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}


void loop() {

  // Woke up before the sensor thus, wait for the data to be ready.
  while(!ccs.available()){
    Serial.println("Waiting for data");
    delay(500);
    };

  // measure the time needed to do the following to reduce it from the actual Deep Sleep time.
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

    // again not sure if I should push them before reading or after (so that they are considered for the next reading).
    // However, this seems to give good results anyway.
    ccs.setEnvironmentalData(tmp_hdc+6, humidity);

    if (eco2 != 0){
          // if the sensor is not giving 0 then send the data.
          // when starting-up the sensor (ccs811), it gives 0 for around 4 readings.
          
          makeHTTPRequest(tmp_hdc, humidity, eco2, tvoc);
          
          // this is used just for debugging so that we don't spam the channel with trash.
          //Serial.println("fake sending...");
          }
    
  }

  Serial.write("Sleep!");
  delay(1000);
  
  // When everything succedes, Deep Sleep for 60 seconds - the time needed to perform all the above.
  ESP.deepSleep((60-(millis() - startTime)/1000-1)*1e6);

  // This will never be executed. It was used only for debugging and checking if the Deep Sleep works.
  delay(100);
  Serial.write("Woke up!");
}
