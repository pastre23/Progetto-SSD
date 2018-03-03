#include <dht11.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

const char* ssid     = "iphoneTremante";
const char* password = "12345678";

const char* host = "172.20.10.3";
const int httpsPort = 80;

static const uint8_t caCert[] PROGMEM = {
  #include "caCert.h"
};

//Variables
int alarm_received = D6;
int pin_wifi_connected = D7;
int gas_alarm = D0;
int dht11_pin = D1;
dht11 DHT;

//Function Setup
void setup() {
    //0) Initializing Serial
    Serial.begin(115200);
    delay(10);
    
    //4) Initializing Pin
    pinMode(alarm_received,OUTPUT);
    pinMode(pin_wifi_connected,OUTPUT);
    pinMode(gas_alarm,INPUT);
    pinMode(dht11_pin,OUTPUT);

    read_dht11(); //fictitious reading

}

void loop() {
  if(digitalRead(gas_alarm)==LOW) { 
    
    //Connecting Wifi
    connectToWiFi();
    digitalWrite(pin_wifi_connected,HIGH); //turn on the led

    // Synchronize time useing SNTP. This is necessary to verify that
    // the TLS certificates offered by the server are currently valid.
    SynchronizeSNTP(); 

    Serial.println(WiFi.localIP()); //Print the IP address

    //Create client to begin a connection
    WiFiClientSecure client;
  
    //Load root certificate in DER format into WiFiClientSecure object
    bool res = client.setCACert_P(caCert, sizeof(caCert));
    if (!res) {
      Serial.println("Failed to load root CA certificate!");
      while (true) 
       yield();  
    } else 
      Serial.println("Success to load root CA certificate!");    

    Serial.println("Client Ready!!");
    
    // Connect to remote server
    Serial.print("connecting to ");
    Serial.println(host);
    if (!client.connect(host, httpsPort)) {      
      Serial.println("connection failed");
      WiFi.disconnect(); //Disconnect WiFi
      digitalWrite(pin_wifi_connected,LOW);
      return;
    } else  
       Serial.println("Connected to server!");    
  
    // Verify validity of server's certificate
    if (client.verifyCertChain(host))  {
      Serial.println("Server certificate verified");
    } else {
      WiFi.disconnect(); //Disconnect WiFi
      digitalWrite(pin_wifi_connected,LOW);
      Serial.println("ERROR: certificate verification failed!");
      return;
    }


    //Read data from temperature sensor
    read_dht11();
  
    //prepare message
    String request = "REQUEST: GAS_ALARM\n";
    request += "Temp: " + String(DHT.temperature) + "\n";
    request += "Humidity: " + String(DHT.humidity) + "\n";
    request += "Host: " + (String)host + "\n";
    request += "User-Agent: ESP8266\n\r";
  
    //send message
    client.print(request);
    Serial.println("request sent");

     while (client.connected()) {
        String response = client.readStringUntil('\r');
        Serial.println(response);
        if (response.indexOf("GAS_ALARM_DETECTED") != -1) {
          Serial.println("Alarm received");
          //Turn on green led
          digitalWrite(alarm_received,HIGH);
          delay(3000);   
          digitalWrite(alarm_received,LOW);
        }
        break;
      }
  
      //Disconnect WiFi
      WiFi.disconnect();
      digitalWrite(pin_wifi_connected,LOW);
  }
  else //digitalRead(gas_alarm)==HIGH
    Serial.println("No Gas alarm detected!!");
  
  delay(1000);
   
}



//Funzioni di servizio

void connectToWiFi() {
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}


void SynchronizeSNTP() {
  // Synchronize time useing SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void read_dht11() {
  int chk;
  Serial.print("Reading DHT11..., \t");
  chk = DHT.read(dht11_pin);    // READ DATA
  switch (chk){
    case DHTLIB_OK:  
                Serial.print("OK,\t"); 
                break;
    case DHTLIB_ERROR_CHECKSUM: 
                Serial.print("Checksum error,\t"); 
                break;
    case DHTLIB_ERROR_TIMEOUT: 
                Serial.print("Time out error,\t"); 
                break;
    default: 
                Serial.print("Unknown error,\t"); 
                break;
  }
}




