#include <ESP8266WiFi.h>
#include <FS.h>
#include <time.h>
#include <LiquidCrystal.h>

const char* ssid     = "iphoneTremante";
const char* password = "12345678";

static const uint8_t x509[] PROGMEM = {
  #include "serverCert.h"
};

static const uint8_t rsakey[] PROGMEM = {
  #include "serverKey.h"
};

int pin_alarm = D8;
int pin_wifi_connected = D7;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = D6, en = D5, d4 = D4, d5 = D3, d6 = D2, d7 = D1;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServerSecure server(80);

void setup() {
    Serial.begin(115200);
    delay(10);

    //Connecting Wifi
    connectToWiFi();
    digitalWrite(pin_wifi_connected,HIGH);

    //Set certificate and private key
    server.setServerKeyAndCert_P(rsakey, sizeof(rsakey),
                                  x509, sizeof(x509));

    // Start the server
    server.begin();
    Serial.println("Server started");
    
    // Print the IP address
    Serial.println(WiFi.localIP());

    // Synchronize time useing SNTP.
    SynchronizeSNTP();

    //4) Initializing Pin
    pinMode(pin_alarm,OUTPUT);
    pinMode(pin_wifi_connected,OUTPUT);

    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("NO ALARM");
  lcd.setCursor(0, 1);
  lcd.print("DETECTED!");

  // Check if a client has connected
  WiFiClientSecure client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  unsigned long timeout = millis() + 3000;
  while(!client.available() && millis() < timeout){
    delay(1);
  }
  if (millis() > timeout) {
    Serial.println("timeout");
    client.flush();
    client.stop();
    return;
  }
  
  // Read and Print the request from CLIENT
  String req = client.readStringUntil('\r');
  Serial.println(req);
  Serial.println();
  client.flush();

  //Building response
  String response = "RESPONSE: ";

  // Send the response to the client
  if (req.indexOf("GAS_ALARM") != -1) {

    response += "GAS_ALARM_DETECTED\n";

    String temperature = "Temp: " + readUntil(req.substring(25),"\n");
    String humidity = "Humidity: " + readUntil(req.substring(38),"\n");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(temperature);
    lcd.setCursor(0, 1);
    lcd.print(humidity);
    send_alarm();

  } else {

    response += "INVALID_REQUEST\n";

  }

  String host = IpAddress2String(WiFi.localIP());
  response += "Host: " + host + "\n\r";
  client.print(response); //send response

  delay(1);
  Serial.println("Client disconnected");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NO ALARM");
  lcd.setCursor(0, 1);
  lcd.print("DETECTED!");
}



//Funzioni di servizio
void connectToWiFi()
{
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


void send_alarm() 
{
  for(int i = 0; i<10; i++) {

    digitalWrite(pin_alarm,HIGH);
    delay(500);
    digitalWrite(pin_alarm,LOW);
    delay(500);    
       
  }
}



String readUntil(String stringa1,String carattere) {

  int i = 1;
  bool found = false;
  String stringa3 = "";
  stringa3.concat(stringa1[0]);
  while(i<5 && !found)
  {
    if (String(stringa1[i]).equals(carattere)) {
      found = true;
    } else {
      stringa3.concat(stringa1[i]);
    }
    i++;
    
  }

  return stringa3;

}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}


