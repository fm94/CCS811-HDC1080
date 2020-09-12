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

const char *ssid = "";
const char *password = "";

const char* apiKey = "";
const char* resource = "/update?api_key=";
const char* server = "api.thingspeak.com";

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.write("Setup!");
  gpio_init();
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);

  while(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
    delay(1000);
  }

  Serial.println("set 60 secs");
  ccs.setDriveMode(CCS811_DRIVE_MODE_60SEC);
  delay(100);


  hdc1080.begin(0x40);

  tmp_hdc = hdc1080.readTemperature() - 6;
  humidity = hdc1080.readHumidity();
  ccs.setEnvironmentalData(tmp_hdc+6, humidity);

  sleep();
  delay(100);
  
  //Serial.println("enable interrupts");
  //ccs.enableInterrupt();
  //delay(1000);

  //sleep();
  //delay(100);
}

void callback() {
  Serial1.println("Callback");
  Serial.flush();
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

  int timeout = 20 * 4; // 10 seconds
  while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() != WL_CONNECTED) {
     Serial.println("Failed to connect wifi, going back to sleep");
  }

  Serial.print("IP address: "); 
  Serial.println(WiFi.localIP());
}

void sleep()
{
//    wifi_station_disconnect();
//    wifi_set_opmode_current(NULL_MODE);
//    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
//    wifi_fpm_open();
//    gpio_pin_wakeup_enable(GPIO_ID_PIN(12), GPIO_PIN_INTR_LOLEVEL); // or LO/ANYLEVEL, no change
//    wifi_fpm_do_sleep(8*1000);
//    delay(1);

    uint32_t sleep_time_in_ms = 50000;
    wifi_set_opmode(NULL_MODE);
    wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_set_wakeup_cb(callback);
    wifi_fpm_do_sleep(sleep_time_in_ms *1000 );
    delay(sleep_time_in_ms + 1);
}

void loop() {
  
  while(!ccs.available()){
    Serial.println("Waiting for data");
    delay(500);
    };

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
          //Serial.println("fake sending...");
          }
    
  }

  Serial.write("Sleep!");
  //delay(1000);
  sleep();
  delay(100);
  Serial.write("Woke up!");
}
