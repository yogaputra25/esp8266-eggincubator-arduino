// https://learn.adafruit.com/esp8266-temperature-slash-humidity-webserver/code

#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

// Heating mat (relay)
#define heatPin 13
#define humPin 14


// Stepper
#define HALFSTEP 8  
  
#define motorPin1  2     
#define motorPin2  0     
#define motorPin3  D0    
#define motorPin4  D8    

// NOTE: The sequence 1-3-2-4 is required for proper sequencing of 28BYJ-48  
AccelStepper stepper(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4); 

// DHT temperature & humidity
#define DHTTYPE DHT11  
DHT dht(12, DHTTYPE);

float hum;
float temp;
char temp_str[6];
char hum_str[6];
const char* heat_str;

// Interval for DHT sensor
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor

// Interval for turning eggs
unsigned long turnPreviousMillis = 0;        // will store last temp was read
const long turnInterval = 108000;             // 3 hours

// Interval for toggling heater
unsigned long heatPreviousMillis = 0;        
const long heatInterval = 10000;

// Interval for toggling humidity
unsigned long humPreviousMillis = 0;        
const long humInterval = 10000;             

// Webserver
const char* ssid = "HGU-Entok_Jali";
const char* password = "12345678991";

ESP8266WebServer server(80);

void getTemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;

// Time
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;  
  int dy = hr / 24;  
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    hum = dht.readHumidity();          // Read humidity (percent)
    temp = dht.readTemperature();     // Read temperature
    Serial.println("Temperature: ");
    Serial.println(temp);
    Serial.println("Humidity: ");
    Serial.println(hum);
    Serial.println("-------------");
    
    Serial.println(sec % 60);
    Serial.println(min % 60);
    Serial.println(hr % 24);

    
  lcd.setCursor(0, 0);
  lcd.print("S:");
  lcd.print(temp);
   
   lcd.setCursor(9, 0); 
   lcd.print(hr % 24);
   lcd.print(":");
   lcd.print(min % 60);
   lcd.print(":");
   lcd.print(sec % 60);
  
  
 
  // Pindahkan kursor ke baris berikutnya dan cetak lagi
  lcd.setCursor(0, 1);      
  lcd.print("K:");
  lcd.print(hum);
  lcd.setCursor(9, 1); 
   lcd.print(dy);
   lcd.print(" Hari");
  
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}

void setup(void){
  Serial.begin(9600);
  lcd.begin();
  WiFi.begin(ssid, password);
  Serial.println("");

  // Heating mat
  pinMode(heatPin, OUTPUT);

  // Humidity mat
  pinMode(humPin, OUTPUT);

  // Stepper
  stepper.setMaxSpeed(10000000);
  stepper.setSpeed(10000); 

  // Dht
  dht.begin();

  // Server
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {

  // DHT
  getTemperature();
  dtostrf(temp, 5, 2, temp_str);
  dtostrf(hum, 5, 2, hum_str);

  String title = "EGGBoT";
  String cssClass = "mediumhot";

  if (temp < 0)
    cssClass = "cold";
  else if (temp > 37)
    cssClass = "hot";


  // Time
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;  

  String message = "<!DOCTYPE html><html><head><title>" + title + "</title><meta charset=\"utf-8\" /><meta name=\"viewport\" content=\"width=device-width\" /><style>\n";
  message += "html {height: 100%;}";
  message += "div {color: #fff;font-family: 'Advent Pro';font-weight: 400;left: 50%;position: absolute;text-align: center;top: 50%;transform: translateX(-50%) translateY(-50%);}";
  message += "h2 {font-size: 90px;font-weight: 400; margin: 0}";
  message += "body {height: 100%;}";
  message += ".cold {background: linear-gradient(to bottom, #7abcff, #0665e0 );}";
  message += ".mediumhot {background: linear-gradient(to bottom, #81ef85,#057003);}";
  
      
  message += "</style></head><body class=\"" + cssClass + "\"><div><h2>" + title +  "</h2><h4>Waktu:" + hr + ":" + min % 60 + ":" + sec % 60 +" </h4><h3>Suhu: " + temp_str + "&nbsp;<small>&deg;C</small></h3><h3>Kelembaban: " + hum_str + "&nbsp;%</h3><h3>Pemanas: " + heat_str + "&nbsp;</h3><label>Suhu Mati:</label><input type='text' id='fname' name='fname'><br><br><label>Kelembaban Mati:</label><input type='text' id='lname' name='lname'><br><br><input type='submit' value='Submit'></div></body></html>"; //ru
  //message += "</style></head><body class=\"" + cssClass + "\"><div><h2>" + title +  "</h2><h4>Running time:" + hr + ":" + min % 60 + ":" + sec % 60 +" </h4><h3>Temperature: " + temp_str + "& nbsp;<small>&deg;C</small></h3><h3>Humidity: " + hum_str + "&nbsp;%</h3><h3>Heating: " + heat_str + "&nbsp;</h3></div></body></html>"; //en
  server.send(200, "text/html", message);
  });
 
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("incubator")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  
  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");
  
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

   
  
}

void rotateEggs(){

  unsigned long currentMillis = millis();

  if(currentMillis - turnPreviousMillis >= turnInterval) {

    turnPreviousMillis = currentMillis;

    stepper.move(2000);
    

    Serial.println("Turning eggs!");
  }

  stepper.run();
}

void toggleHeater(){

  unsigned long currentMillis = millis();

  if(currentMillis - heatPreviousMillis >= heatInterval) {

    heatPreviousMillis = currentMillis;

    getTemperature();

    if((temp + 0.5) < 37){
      digitalWrite(heatPin, LOW);
      Serial.println("Heater on!");
      heat_str = ("ON"); 
    }
    else if ((temp - 0.5) > 37)  {
      digitalWrite(heatPin, HIGH);
      Serial.println("Heater off!");
      heat_str = ("OFF");
    }
  
    
    if((hum + 2) < 53){
      digitalWrite(humPin, LOW);
      Serial.println("Humidity on!");
   
    }
    else if ((hum - 2) > 53)  {
      digitalWrite(humPin, HIGH);
      Serial.println("Humidity off!");
    
    }
  }
}

void loop(void){
  server.handleClient();
  rotateEggs();
  toggleHeater();
}
