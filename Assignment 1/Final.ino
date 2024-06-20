/* 
 * ****************************************************************************
 *  University of Milano-Bicocca
 *  DISCo - Department of Informatics, Systems and Communication
 *  Viale Sarca 336, 20126 Milano, Italy
 *
 *  Copyright © 2019-2024 by:
 *    Andrea Pascuzzi - a.pascuzzi2@campus.unimib.it
 *    Emanuele Dubini   - e.dubini1@campus.unimib.it
 *    
 * ****************************************************************************
 *
 * First Assignment - Iot Laboratory
 *
 *
 * Libraries:
 *   - requires Frank de Brabander's 'LiquidCrystal I2C' library, install it from the Arduino IDE library manager
 *       https://github.com/johnrickman/LiquidCrystal_I2C
 *   - requires the Adafruit's 'DHT sensor library', install it and its dependencies from the Arduino IDE library manager
 *
 */



// include WiFi library
#include <ESP8266WiFi.h>
#include "secrets.h"

// MySQL libraries
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

// include Web Server library
#include <ESP8266WebServer.h>

// include LCD library
#include <LiquidCrystal_I2C.h>   // display library
#include <Wire.h>                // I2C library

// include DHT library
#include <DHT.h>  //Humidity sensor



// WiFi signal strength threshold
#define RSSI_THRESHOLD -60            

// LCD setup
#define DISPLAY_CHARS 16    // number of characters on a line
#define DISPLAY_LINES 2     // number of display lines
#define DISPLAY_ADDR 0x27   // display address on I2C bus

LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES);   // display object
unsigned long lastLcdRefresh = millis();   // init with millis at first call, numero di millisecondi da quando è stato alimentato il processore/ la scheda

// Button setup
// Button for the activation e deactivation of the alarm
#define BUTTON D3                  // button pin, eg. D1 is GPIO5 and has an optional internal pull-up
#define BUTTON_DEBOUNCE_DELAY 20   // button debounce time in ms

// DHT setup
#define DHTPIN D7       // sensor I/O pin
#define DHTTYPE DHT11   // sensor type DHT 11
#define BUZZER_DHT D0   // buzzer pin for DHT sensor
unsigned long buzzerStartTime = 0; // Variabile per memorizzare il tempo di inizio del suono del buzzer
bool buzzerOn = false; // Variabile per tenere traccia dello stato del buzzerù
int DhtRead_count = 1;

// Initialize DHT sensor
DHT dht = DHT(DHTPIN, DHTTYPE);

// TILT sensor setup
#define TILT D5           // SW-520D pin, eg. D1 is GPIO5 and has optional internal pull-up
#define LED_TILT D6   // LED_TILT pin
#define debounceDelay_TILT 3000 // Tempo di debounce in millisecondi per il tilt sensor
unsigned long lastDebounceTime_TILT = 0;

#define PHOTORESISTOR A0              // photoresistor pin
#define PHOTORESISTOR_THRESHOLD 700   // turn led on for light values lesser than this

//LED green and red for Alarm
#define LED_GREEN_ALARM 1   // green LED for when alarm is active
#define LED_RED_ALARM D8   
//Variable Alarm
boolean Alarm = true; //Button to deactivate the alarm. If True the alarm is active, and includes all the alerts from the various sensors. Deactivating it deactivates everyone

//others config
#define LED_FOTORESISTOR D4   // LED_FOTORESISTORpin
bool firstIteration_LCD = true;   // variable for the initialization of the lcd, to be made once
bool firstIteration_Values = true;   //variable for the first read of the sensors, to be displayed


/*
     Other Variables. Variables need to be global for the code to properly work and trasmit them to the database.
*/

// WiFi cfg
char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
#ifdef IP
IPAddress ip(IP);
IPAddress subnet(SUBNET);
IPAddress dns(DNS);
IPAddress gateway(GATEWAY);
#endif
WiFiClient client; //wifi client class that offers methods to connect to wifi
int signalStrenght;

// MySQL server cfg
char mysql_user[] = MYSQL_USER;       // MySQL user login username
char mysql_password[] = MYSQL_PASS;   // MySQL user login password
IPAddress server_addr(MYSQL_IP);      // IP of the MySQL *server* here
MySQL_Connection conn((Client *)&client);

char query[256];    //query per sql
char INSERT_DATA[] = "INSERT INTO `%s`.`assignment_1` (`ssid`, `rssi`,`tilt`, `humidity`, `temperature`, `light`, `alarm`, `warning`) VALUES ('%s',%d,%d, %f, %f, %d, %d, %d)"; //Template for the standard query to the DB

//light Sensor cfg
static unsigned int lightSensorValue;
int LightRead_count = 1;

// Web Server config
ESP8266WebServer server(80);   // HTTP server on port 80
// Declare a WiFiClient object to handle WiFi connections and an ESP8266WebServer object to handle HTTP requests on port 80.


//Variables for the DHT Humidity and Temperature sensor.
unsigned long previousMillisDHT = millis();
float h = 0; //DHT sensor humidity
float t= 0; //DHT sensor temperature
float hic; //heat index in Celsius

// Variable for TILT sensor
byte tilt = digitalRead(TILT);   // read the SW-520D state


//Define used for the timing of sensors and DB interactions
#define intervalDHT 5000  // interval between DHT sensor measurements in milliseconds
#define LIGHT_READ_INTERVAL 5000         // read light sensor each 5000 ms
#define WIFI_STATUS_INTERVAL 5000         // read WIFI status each 5000 ms
#define LCD_REFRESH_INTERVAL 5000         // refresh LCD display each 5000 ms
#define DB_WRITE_INTERVAL 15000       // Interval of writing on DB, every 10 seconds
/* the values are set for the demo, for real use, for monitoring a greenhouse it would be recommended to set the reading of the various sensors every 20 minutes
and writing to the DB every hour, reported commented below*/

/*
#define intervalDHT 1200000  // interval between DHT sensor measurements in milliseconds
#define LIGHT_READ_INTERVAL 1200000         // read light sensor each 1200000 ms
#define WIFI_STATUS_INTERVAL 1200000         // read WIFI status each 1200000 ms
#define LCD_REFRESH_INTERVAL 1200000         // refresh LCD display each 20 minutes
#define DB_WRITE_INTERVAL 3600000       // Interval of writing on DB, every hour
*/




void setup() {

  // set Alarm LED pins as outputs
  pinMode(LED_GREEN_ALARM, OUTPUT); // Set the green led pin as output
  pinMode(LED_RED_ALARM, OUTPUT); // Set the red led pin as output

  // turn led off
  digitalWrite(LED_GREEN_ALARM, HIGH);
  digitalWrite(LED_RED_ALARM, HIGH);

  // set LED pin as outputs
  pinMode(LED_FOTORESISTOR, OUTPUT);


  // turn led off
  digitalWrite(LED_FOTORESISTOR, HIGH);

  // set WIFI
  WiFi.mode(WIFI_STA); 
  //wifi in client mode so that you specify that you want to receive a connection from someone else
  //it is possible to configure the esp also as an access point so that you can connect and control the esp

  // set Web Server
  //Match different HTTP requests to specific functions. When an HTTP request arrives on the root (/),
  //the handle_root function is called. If the request is /ON, handle_ledon is called,
  //while for /OFF handle_ledoff is called.
  //If no matching path is found, handle_NotFound is called.
  server.on("/", handle_root);
  server.on("/ON", handle_ledon);
  server.on("/OFF", handle_ledoff);
  server.onNotFound(handle_NotFound);
  //start the web server on port 80
  server.begin(); 
  Serial.println(F("HTTP server started"));

  // set LCD
  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(F("LCD found."));
    lcd.begin(DISPLAY_CHARS, 2);   // initialize the lcd

  } else {
    Serial.print(F("LCD not found. Error "));
    Serial.println(error);
    Serial.println(F("Check connections and configuration. Reset to try again!"));
    while (true)
      delay(1);
  }


  // DHT sensor
  dht.begin();
  // set buzzer pin as outputs. This buzzer is used for DHT sensor
  pinMode(BUZZER_DHT, OUTPUT);
  // turn buzzer off
  digitalWrite(BUZZER_DHT, HIGH);
  // set BUTTON pin as input with pull-up
  pinMode(BUTTON, INPUT_PULLUP);

  //TILT sensor
    // set LED_TILT pin as outputs
  pinMode(LED_TILT, OUTPUT);
  // set TILT pin as input
  pinMode(TILT, INPUT);
  // turn LED_TILT off
  digitalWrite(LED_TILT, HIGH);

  Serial.begin(115200);
  Serial.println(F("\n\nSetup completed.\n\n"));
}




void loop() {

  //////// wifi section ///////
  wifiConnection();

  //////// first reads section, it compile once and get the values from all sensors. it is used to enable the reading for longer intervals, as for the first iteraction the values are already measured ///////
  if(firstIteration_Values){
    First_Values();

    //initLCD is executed at the first execution of the loop(), after which refreshLCD() is executed every 1200000 ms    
    initLCD();
    firstIteration_LCD = false;
    firstIteration_Values = false;
  }

  //////// WiFi strenght section ///////
  readWifiStatus(); 
  

  //////// LIGHT SENSOR section ///////
  readLightSensor();

  
  //////// DHT SENSOR section ///////
  if (millis() - previousMillisDHT >= intervalDHT) {  // controlla se è passato l'intervallo
    DHT_read();       //Aggiornamento valori DHT
  }

//Section that enables the Buzzer
   if ((h > 75 || t > 30) && Alarm == true) {
     // Turn on the buzzer if the humidity exceeds 75% or the temperature exceeds 30 degrees
     digitalWrite(BUZZER_DHT, LOW); // turn on the buzzer
   }

   if ((digitalRead(BUZZER_DHT) == LOW && h < 75 && t < 30) || Alarm == false) {
     // Turn off the buzzer if it is on and the above conditions are not met
     digitalWrite(BUZZER_DHT, HIGH); // turn off the buzzer
   }
  //////// TILT SENSOR section ///////
  TILT_read();    // Continuous update of the tilt sensor status

  //as soon as the loop is started refreshLCD() is not executed immediately, as soon as a few seconds have passed it starts to be executed
  refreshLCD();

  //Function that manages the alarm button
  buttonAlarm();

  //Updating the alarm status LED
  ledAlarm();

  //////// WEB SERVER section ///////
  server.handleClient();   // listening for clients on port 80

  //Write to SQL DB
  WriteMultiToDB(ssid, signalStrenght, tilt, h, t, lightSensorValue, Alarm);   // write on MySQL table if connection works

}

void buttonAlarm(){
  if (isButtonPressed() == true) {   // Button pressed
    Alarm = !Alarm;
    if (Alarm == LOW) {
      Serial.println(F("Button: PRESSED, Buzzer OFF"));
      displaySystemOff();
      
    } else {
      Serial.println(F("Button: PRESSED, Buzzer ON"));
      displaySystemOn();
    } 
  }
}

void displaySystemOn(){
  // clear display and write the message
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Monitoring ON");
      lcd.setCursor(0, 1);
      lcd.print("System active");

      lastLcdRefresh = millis();
}

void displaySystemOff(){
  // clear display and write the message
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Monitoring OFF");
      lcd.setCursor(0, 1);
      lcd.print("System inactive");

      lastLcdRefresh = millis();
}

void ledAlarm(){
     if (Alarm) { // If the alarm is active
     digitalWrite(LED_GREEN_ALARM, HIGH); // Turns on the green LED
     digitalWrite(LED_RED_ALARM, LOW); // Turns off the red LED
   } else { // If the alarm is not active
     digitalWrite(LED_GREEN_ALARM, LOW); // Turns off the green LED
     digitalWrite(LED_RED_ALARM , HIGH); // Turns on the red LED
   }
}

void initLCD(){
     lcd.setBacklight(255); // Set the backlight to maximum
     lcd.home(); // Move the cursor to 0.0
     lcd.clear(); // Clear the text
     lcd.print("System starting"); // Show startup text on first line
     lcd.setCursor(0, 1); // Move the cursor to the second line
     for (int i = 0; i < 16; i++) { // Print the loading bar
       lcd.print(".");
       delay(100); // Add a small delay to make the movement of the bar visible
     }

      Serial.print("\nINIZIALIZZO DISPLAY\n");
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("H:");
      lcd.print(h, 1);
      lcd.print("% T:");
      lcd.print(t, 1);
      lcd.print(" C");
      lcd.setCursor(0, 1);

      lcd.print("W:");
      lcd.print(signalStrenght);
      lcd.print("dBm ");

      lcd.print("W:");
      if(tilt == HIGH){
        lcd.print("Open ");
      } else{
        lcd.print("Close");
      }
}

void refreshLCD(){
    
  // Read LIGHT Sensor every LIGHT_READ_INTERVAL milliseconds
  if (millis() - lastLcdRefresh > LCD_REFRESH_INTERVAL) { //if the current time minus the past instant in which I changed the LED state is greater than 500 then I change the LED state

    Serial.print("\nAGGIORNO DISPLAY\n");
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("H:");
      lcd.print(h, 1);
      lcd.print("% T:");
      lcd.print(t, 1);
      lcd.print(" C");
      lcd.setCursor(0, 1);

      lcd.print("W:");
      lcd.print(signalStrenght);
      lcd.print("dBm ");

      lcd.print("W:");
      if(tilt == HIGH){
        lcd.print("Open ");
      } else{
        lcd.print("Close");
      }

    
    lastLcdRefresh = millis();    // update last execution millis
  }
}

void readLightSensor(){
  static unsigned long lastLightRead = millis();   // init with millis at first call, numero di millisecondi da quando è stato alimentato il processore/ la scheda

  // Read LIGHT Sensor every LIGHT_READ_INTERVAL milliseconds
  if (millis() - lastLightRead > LIGHT_READ_INTERVAL) { //se il tempo attuale meno l'istante passato in cui ho cambiato lo stato del led è maggiore a 500 allora cambio stato al led
    
    // for brightness reading we plan to take 3 measurements (each measurement taken every LIGHT_READ_INTERVAL), calculate the average and then save it to the database every DB_WRITE_INTERVAL (use 1 hour for real use)
    // for real use defined to measure brightness every 20 minutes and made the third measurement, every 60 minutes write the average to the db.
    if(LightRead_count < 3){
      lightSensorValue = (lightSensorValue + analogRead(PHOTORESISTOR)) / 2;

      LightRead_count++;
    } else{
      lightSensorValue = analogRead(PHOTORESISTOR);   // read analog value (range 0-1023)

      LightRead_count = 1;
    }
    
    // Stampa i risultati
    Serial.print(F("\nlettura "));
    Serial.print(LightRead_count);
    Serial.print(F(" sensore Luminosità "));
    Serial.print(F("\nLight sensor value: "));
    Serial.println(lightSensorValue);

    lastLightRead = millis();  // update last execution millis
  }
    if (lightSensorValue <= PHOTORESISTOR_THRESHOLD && Alarm == true) {   // high brightness
      digitalWrite(LED_FOTORESISTOR, LOW);                           // LED_FOTORESISTOR on
    } else {                                             // low brightness
      digitalWrite(LED_FOTORESISTOR, HIGH);                            // LED_FOTORESISTOR off
    }
}

void DHT_read() { //Updating DHT values
     previousMillisDHT = millis(); // save the current time as the last measurement time

    // for temperature and humidity reading we plan to take 3 measurements (each measurement taken every intervalDHT), calculate the average and then save it to the database every DB_WRITE_INTERVAL (use 1 hour for real use)
    // for real use you defined to measure temperature and humidity every 20 minutes and made the third measurement, every 60 minutes write the average to the db.
     if(DhtRead_count < 3){

       // Read humidity and temperature
       h = (h + dht.readHumidity()) / 2;
    
       t = (t + dht.readTemperature()) / 2;
    
       if (isnan(h) || isnan(t)) { // check if the read failed
         Serial.println(F("\nUnable to read from DHT sensor!"));
         return;
       }

       hic = (hic + dht.computeHeatIndex(t, h, false)) / 2;

       DhtRead_count++;

     }else{
        h = dht.readHumidity(); // humidity in percentage, between 20 and 80% (accuracy ±5%)
        t = dht.readTemperature(); // temperature in Celsius, between 0 and 50°C (accuracy ±2°C)
        hic = dht.computeHeatIndex(t, h, false);

        DhtRead_count = 1;
      
     }

     // Print the results
     Serial.print(F("\nreading "));
     Serial.print(DhtRead_count);
     Serial.print(F(" DHT sensor\n"));
     Serial.print(F("Humidity: "));
     Serial.print(h);
     Serial.print(F("% Temperature: "));
     Serial.print(t);
     Serial.print(F("°C Apparent temperature: ")); // the temperature perceived by humans (considers humidity)
     Serial.print(hic);
     Serial.println(F("°C"));
}


void wifiConnection(){
  // connect to WiFi (if not already connected)
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("\nConnecting to SSID: "));
    Serial.println(SECRET_SSID);

#ifdef IP //if constant IP is defined then assign static IP, DNS, gateway and subnet (parameters defined in the secrets.h file)
  WiFi.config(ip, dns, gateway, subnet);   // by default network is configured using DHCP
#endif

    Serial.print("Connessione alla rete WIFI\n");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(F("."));
      delay(250);
    }
    signalStrenght = WiFi.RSSI();
    Serial.println(F("\nConnected!"));
    Serial.print(F("IP Address: "));
    Serial.println(WiFi.localIP());
  }
}


void readWifiStatus() {
static unsigned long lastWifistatus = millis();   // init with millis at first call

// Read Wifi status every WIFI_STATUS_INTERVAL milliseconds
  if (millis() - lastWifistatus > WIFI_STATUS_INTERVAL) { //if the current time minus the past instant in which I changed the LED state is greater than 500 then I change the LED state
    
  Serial.println(F("\n=== WiFi connection status ==="));

  // SSID
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // signal strength
  signalStrenght = WiFi.RSSI();
  Serial.print(F("Signal strength (RSSI): "));
  Serial.print(signalStrenght);
  Serial.println(" dBm");

  // current IP
  
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());
  Serial.println(F("==============================\n"));

  lastWifistatus = millis();                               // update last execution millis
  }
}

void TILT_read() {
   // Read the current state of the tilt sensor
   int currentTiltState = digitalRead(TILT);

   // Check if the tilt sensor status has changed and if the debounce time has elapsed
   if (currentTiltState != tilt && millis() - lastDebounceTime_TILT > debounceDelay_TILT) {
     // Update the time of the last debounce
     lastDebounceTime_TILT = millis();
    
     // Update the tilt variable
     tilt = currentTiltState;

     // Check if the tilt sensor is tilted and the alarm is active
     if (tilt == HIGH && Alarm == true) {
       digitalWrite(LED_TILT, HIGH); // Turn on the LED_TILT
     } else {
       digitalWrite(LED_TILT, LOW); // Turn off the LED_TILT
     }
   }
}


boolean isButtonPressed() {
  static byte lastState = digitalRead(BUTTON);   // the previous reading from the input pin

  for (byte count = 0; count < BUTTON_DEBOUNCE_DELAY; count++) {
    if (digitalRead(BUTTON) == lastState) return false;
    delay(1);
  }

  lastState = !lastState;
  return lastState == HIGH ? false : true;
}



void handle_root() {
  Serial.print(F("New Client with IP: "));
  Serial.println(server.client().remoteIP().toString());
  server.send(200, F("text/html"), SendHTML(Alarm));
}

void handle_ledon() {
  Alarm = true; // activate alarm
  displaySystemOn();
  Serial.println(F("Led ON"));
  server.send(200, F("text/html"), SendHTML(Alarm));
}

void handle_ledoff() {
  Alarm = false; // deactivate alarms
  displaySystemOff();
  Serial.println(F("Led OFF"));
  server.send(200, F("text/html"), SendHTML(Alarm));
}

void handle_NotFound() {
  server.send(404, F("text/plain"), F("Not found"));
}

String SendHTML(uint8_t ledstat) {
  String ptr = F("<!DOCTYPE html> <html>\n"
     "<head><meta http-equiv=\"refresh\" content=\"30\" name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n"
     "<title>Web Alarm Control</title>\n"
     "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n"
     "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n"
     ".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n"
     ".button-on {background-color: #1abc9c;}\n"
     ".button-on:active {background-color: #16a085;}\n"
     ".button-off {background-color: #ff4133;}\n"
     ".button-off:active {background-color: #d00000;}\n"
     "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n"
     "</style>\n"
     "</head>\n"
     "<body>\n"
     "<h1>Turning Alarm ON/OFF </h1>\n");

  if (ledstat) {
    ptr += F("<p>Current Alarm Status: ON</p><a class=\"button button-off\" href=\"/OFF\">OFF</a>\n");
  } else {
    ptr += F("<p>Current Alarm Status: OFF</p><a class=\"button button-on\" href=\"/ON\">ON</a>\n");
  }

// Section to show the values read by the sensors
   ptr += F("<h3>Sensor Values</h3>\n");
   ptr += F("<p>Humidity: ");
   ptr += String(h); // Add humidity value
   ptr += F("%</p>\n");

   ptr += F("<p>Temperature: ");
   ptr += String(t); // Add the temperature value
   ptr += F(" C</p>\n");

   ptr += F("<p>Perceived Temperature: ");
   ptr += String(hic); // Add the perceived temperature value
   ptr += F(" C</p>\n");

   ptr += F("<p>WiFi Signal Strength: ");
   ptr += String(signalStrenght); // Add the WiFi signal strength value
   ptr += F(" dBm</p>\n");

   ptr += F("<p>Light Sensor Value: ");
   ptr += String(lightSensorValue); // Add the brightness value
   ptr += F("</p>\n");


  ptr += F("</body>\n"
           "</html>\n");
  return ptr;
}

int WriteMultiToDB(char ssid[], int signalStrenght, int tilt, float h, float t, int lightSensorValue, int Alarm) {
//alarm indicates if the monitoring system is active, warning indicates if a danger is present (such as a value detected above a certain threshold)
   static int warning = false;
   static unsigned long lastDbWrite = millis(); // init with millis at first call, number of milliseconds since the processor/board was powered up
  
  if (millis() - lastDbWrite > DB_WRITE_INTERVAL) { 

  // connect to MySQL
  if (!conn.connected()) {
    conn.close();
    Serial.println(F("\nConnecting to MySQL..."));
    if (conn.connect(server_addr, 3306, mysql_user, mysql_password)) {
      Serial.println(F("MySQL connection established."));
    } else {
      Serial.println(F("MySQL connection failed."));
      return -1;
    }
  }

  if((h > 75 || t > 30) && Alarm == 1){
    warning = true;
  } else{
    warning = false;
  }

  // log data
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  sprintf(query, INSERT_DATA, mysql_user, ssid, signalStrenght, tilt, h, t, lightSensorValue, Alarm, warning);
  Serial.print("\n");
  Serial.println(query);
  Serial.println(F("query lenght: "));
  Serial.println(strlen(query));
  // execute the query
  cur_mem->execute(query);
  // Note: since there are no results, we do not need to read any data
  // deleting the cursor also frees up memory used
  delete cur_mem;
  Serial.println(F("Data recorded on MySQL"));
  
  lastDbWrite = millis();
  warning = false;

  return 1;
}
  return 0; //if it didn't run the timer code
}

void First_Values() {
// humidity and temperature
       // Read humidity and temperature
     h = dht.readHumidity(); // humidity in percentage, between 20 and 80% (accuracy ±5%)
     t = dht.readTemperature(); // temperature in Celsius, between 0 and 50°C (accuracy ±2°C)

    if (isnan(h) || isnan(t)) {   // verify if reading failed
      Serial.println(F("\nImpossibile leggere dal sensore DHT!"));
      return;
    }

// Calculate the heat index in Celsius (isFahreheit = false)
    hic = dht.computeHeatIndex(t, h, false);

// Print the results
    Serial.print(F("\nLettura 1 sensore DHT\n"));
    Serial.print(F("Umidità: "));
    Serial.print(h);
    Serial.print(F("%  Temperatura: "));
    Serial.print(t);
    Serial.print(F("°C  Temperatura apparente: "));   // percepted temperature
    Serial.print(hic);
    Serial.println(F("°C"));

    //TILT
    tilt = digitalRead(TILT);

  //signal strenght
    signalStrenght = WiFi.RSSI();
    Serial.print(F("First Signal strength (RSSI): "));
    Serial.print(signalStrenght);
    Serial.println(" dBm\n");

  // light sensor value
  Serial.print(F("\nLettura 1 sensore Luminosità\n"));
  lightSensorValue = analogRead(PHOTORESISTOR);   // read analog value (range 0-1023)
  Serial.print(F("Light sensor value: "));
  Serial.println(lightSensorValue);

}