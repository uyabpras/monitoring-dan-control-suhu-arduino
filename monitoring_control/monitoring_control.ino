#include <WiFi.h>         // library for Wifi
#include <PubSubClient.h> // library for MQTT
#include "DHT.h"          // Library for DHT sensors
#include <ArduinoJson.h>  // library for json parsing
#include <NTPClient.h>    // library for Get time
#include <Arduino.h>      // library for basic arduino
#include <IRremoteESP8266.h>  // library for IR
#include <IRsend.h>           // library for IR
 
#define wifi_ssid "Alpha"         //wifi ssid
#define wifi_password "beta1234"     //wifi password
 
#define mqtt_server "172.104.160.232"  // server name or IP
#define mqtt_port 1883    //Port MQTT
#define mqtt_user ""     // username
#define mqtt_password ""   // password
 
#define data_topic "topic/data/ruang1"       //Topic Publish
#define debug_topic "debug"                   //Topic for debugging
#define controlsuhu_topic  "topic/control_suhu"   //Topic Subscriber
 
 
bool debug = true;             //Display log message if True
 
#define DHTPIN 13               // DHT Pin 
#define DHTTYPE DHT11           // DHT 11 
const uint16_t kIrLed = 4;  // IR Pin. Recommended: 4 (D2).


 
// Create objects
DHT dht(DHTPIN, DHTTYPE);     
WiFiClient espClient;
PubSubClient client(espClient);
long currentTime, lastTime;
char messages[50];
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

 
void setup() {
  Serial.begin(9600);     
  setup_wifi();                           //Connect to Wifi network
   
  client.setServer(mqtt_server, mqtt_port);    // Configure MQTT connection, change port if needed.
  client.setCallback(callback);
  
  if (!client.connected()) {
    reconnect();
  }
    irsend.begin();
  #if ESP8266
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  #else  // ESP8266
    Serial.begin(115200, SERIAL_8N1);
  #endif  // ESP8266
  
  timeClient.begin();
  timeClient.setTimeOffset(3600*7);
  
  dht.begin();   
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.println("Message arrived on topic: ");
  Serial.println(topic);
  String messagedata;
  
  for (int i = 0; i < length; i++) {
    messagedata += (char)message[i];
  }

  Serial.println(messagedata);

  //Subscribe method
   if (String(topic) == controlsuhu_topic) {
    Serial.print("Changing output to ");
    if(messagedata == "22"){ 
    irsend.sendNEC(0x68D13F41, 32);
    Serial.println("sended data to 22C");
    delay(1000);
    }else if(messagedata == "23"){
    irsend.sendNEC(0x68D13F32, 32);
    Serial.println("sended data to 22C");
    delay(1000);
    }else if(messagedata == "24"){
    irsend.sendNEC(0x68D13F24, 32);
    Serial.println("sended data to 22C");
    delay(1000);
    }else if(messagedata == "25"){
    irsend.sendNEC(0x68D13F55, 32);
    Serial.println("sended data to 22C");
    delay(1000);
    }else if(messagedata == "26"){
    irsend.sendNEC(0x68D13F66, 32);
    Serial.println("sended data to 22C");
    delay(1000);
    }
  }
}

 
//Setup connection to wifi
void setup_wifi() {
  delay(20);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
 
  WiFi.begin(wifi_ssid, wifi_password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
 
 Serial.println("");
 Serial.println("WiFi is OK ");
 Serial.print("=> ESP32 new IP address is: ");
 Serial.print(WiFi.localIP());
 Serial.println("");
}
 
//Reconnect to wifi if connection is lost
void reconnect(){
  while(!client.connected()){
    Serial.print("\nConncting");
    if(client.connect("ESP32Client", mqtt_user, mqtt_password)){
      Serial.print("\nConnected");
      client.subscribe(controlsuhu_topic);
    } else {
      Serial.println("\n Trying to reconnect");
      delay(5000);
    }
  }
}
 
void loop() {
  currentTime = millis();  //function for controll of periodic
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedDate();
   // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
  delay(1000);

 
  // Read temperature in Celcius
    float temp = dht.readTemperature();
  // Read humidity
    float humid = dht.readHumidity();
    String keterangan;
    
    Serial.print("Temperature : ");
    Serial.print(temp);
    Serial.print(" | Humidity : ");
    Serial.print(humid);

    if(temp < 22 && temp > 24){
      keterangan = "tidak ideal";
    }else if(temp >= 22 && temp <= 24){
      keterangan = "ideal";
    }

    if(humid < 45 && humid > 60){
      keterangan = "tidak ideal";
    }else if( humid >= 45 && humid <= 60){
      keterangan = "ideal";
    }
 
  // Nothing to send. Warn on MQTT debug_topic and then go to sleep for longer period.
    if ( isnan(temp) || isnan(humid)) {
      Serial.println("[ERROR] Please check the DHT sensor !");
      client.publish(debug_topic, "[ERROR] Please check the DHT sensor !", true);      // Publish humidity on debug topic
    }
    
    //Parsing Json
    String bufer;
    DynamicJsonDocument doc(1024);
    doc["ruangan" = "ruangan 1";
    doc["date"] = dayStamp;
    doc["time"] = timeStamp;
    doc["temp"] = temp;
    doc["humid"] = humid;
    doc["keterangan"] = keterangan;
    serializeJson(doc, bufer);
     
    // Publish values to MQTT topics
    currentTime = millis();
   if(currentTime - lastTime > 60000*5){      // method Periodic
      client.publish(data_topic, String(bufer).c_str(), true);   // Publish Data
      if ( debug ) {    
       Serial.println("data sent to MQTT.");
       Serial.println(bufer);
     }
      lastTime = millis();
    }  
}
