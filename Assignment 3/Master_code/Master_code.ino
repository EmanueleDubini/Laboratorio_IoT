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
 *
 *
 * Libraries:
 *   - requires Frank de Brabander's 'LiquidCrystal I2C' library, install it from the Arduino IDE library manager
 *       https://github.com/johnrickman/LiquidCrystal_I2C
 *   - requires the Adafruit's 'DHT sensor library', install it and its dependencies from the Arduino IDE library manager
 *   - requisers the tzapu 'WiFiManager' library
 *   - requires the Dushyant Ahuja 'IPGeolocation' library
 *
 */

 /* 
*  The quality of service (QoS) refers to the level of accuracy in the delivery of MQTT messages between senders and the broker, 
*  and between the broker and subscribers, i.e., the accuracy that defines the guarantee of the actual delivery of such messages.
*
*  There are three QoS levels: 
*  - At most once (level 0)
*  - At least once (level 1)
*  - Exactly once (level 2)
*  
*  MQTT QoS essentially pertains to the two phases of message delivery sent from a publishing client and received by a subscribing client 
* (1. the sending from the publisher to the broker; 2. the sending from the broker to the final recipient)
*
*
* Within the code, it has been decided to use:
*  - QoS = 2, for MQTT messages exchanged between the slave and master at startup to obtain necessary communication information (topic strings, DB credentials, ...)
*  - QoS = 1, for MQTT messages exchanged between the slave and master containing sensor readings or actions to be performed
*  - QoS = 0, not used
*/



/*
     INCLUDE SECTION
*/

// include WiFi libraries
#include <ESP8266WiFi.h>
#include "secrets.h"
#include <DNSServer.h>
#include <WiFiManager.h>

// MySQL libraries
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

// include Web Server library
#include <ESP8266WebServer.h>

// include LCD library
#include <LiquidCrystal_I2C.h>  // display library
#include <Wire.h>               // I2C library

// include DHT library
#include <DHT.h>  //Humidity sensor

//Include Json and MQTT librarues
#include <ArduinoJson.h>
#include <MQTT.h>


//PArte aggiuntiva per meteo
#include <ESP8266HTTPClient.h>



/*
     DEFINE SECTION
*/

//Define used for the timing of sensors and DB interactions
#define intervalDHT 5000                 // interval between DHT sensor measurements in milliseconds
#define LIGHT_READ_INTERVAL 5000         // read light sensor each 5000 ms
#define WIFI_STATUS_INTERVAL 5000        // read WIFI status each 5000 ms
#define LCD_REFRESH_INTERVAL 5000        // refresh LCD display each 5000 ms
#define DB_WRITE_INTERVAL 15000          // Interval of writing on DB, every 10 seconds
#define WEATHER_API_READ_INTERVAL 20000  // read weather api value each 20000 ms
/* the values are set for the demo, for real use, for monitoring a greenhouse it would be recommended to set the reading of the various sensors every 20 minutes
and writing to the DB every hour, reported commented below*/

/*
#define intervalDHT 1200000  // interval between DHT sensor measurements in milliseconds
#define LIGHT_READ_INTERVAL 1200000         // read light sensor each 20 minutes
#define WIFI_STATUS_INTERVAL 1200000         // read WIFI status each 20 minutes
#define LCD_REFRESH_INTERVAL 1200000         // refresh LCD display each 20 minutes
#define DB_WRITE_INTERVAL 3600000       // Interval of writing on DB, every hour
#define WEATHER_API_READ_INTERVAL 1800000  // read weather api value each 30 minutes
*/

// WiFi signal strength threshold
#define RSSI_THRESHOLD -60

// LCD setup
#define DISPLAY_CHARS 16   // number of characters on a line
#define DISPLAY_LINES 2    // number of display lines
#define DISPLAY_ADDR 0x27  // display address on I2C bus

// Button setup
// Button for the activation e deactivation of the alarm
#define BUTTON D3                 // button pin, eg. D1 is GPIO5 and has an optional internal pull-up
#define BUTTON_DEBOUNCE_DELAY 20  // button debounce time in ms

// DHT setup
#define DHTPIN D7      // sensor I/O pin
#define DHTTYPE DHT11  // sensor type DHT 11
#define BUZZER_DHT D0  // buzzer pin for DHT sensor


// TILT sensor setup
#define TILT D5                  // SW-520D pin, eg. D1 is GPIO5 and has optional internal pull-up
#define LED_TILT D6              // LED_TILT pin
#define debounceDelay_TILT 3000  // Tempo di debounce in millisecondi per il tilt sensor

#define PHOTORESISTOR A0  // photoresistor pin

//LED green and red for Alarm
#define LED_GREEN_ALARM 1  // green LED for when alarm is active
#define LED_RED_ALARM D8

//others config
#define LED_FOTORESISTOR D4  // LED_FOTORESISTORpin

// MQTT data
#define MQTT_BUFFER_SIZE 512                                  // the maximum size for packets being published and received
#define MQTT_CONTROL "903604/general/#"                       // topic to control the led
#define MQTT_TOPIC_COMUNICATION "903604/master_comunication"  //master comunications towards the slave
#define MQTT_TOPIC_LWT "903604/LWT"
String MQTT_TOPIC_CONTROL = "903604/general/";  // topic to control the led






/*
     Global Variables. Variables need to be global for the code to properly work and trasmit them to the database.
*/

// Threshold global variables
int SogliaTemperatura = 35;  // Default temperature threshold
int SogliaUmidita = 75;      // Default humidity threshold
int SogliaLuce = 700;        // Default light threshold

// WiFi cfg
char ssid[] = SECRET_SSID;  // your network SSID (name)
char pass[] = SECRET_PASS;  // your network password
#ifdef IP
IPAddress ip(IP);
IPAddress subnet(SUBNET);
IPAddress dns(DNS);
IPAddress gateway(GATEWAY);
#endif
WiFiClient client;  //wifi client class that offers methods to connect to wifi
int signalStrenght;

// MySQL server cfg
char mysql_user[] = MYSQL_USER;      // MySQL user login username
char mysql_password[] = MYSQL_PASS;  // MySQL user login password
IPAddress server_addr(MYSQL_IP);     // IP of the MySQL *server* here
MySQL_Connection conn((Client *)&client);

char query[256];                                                                                                                                                                 //query per sql
char INSERT_DATA[] = "INSERT INTO `%s`.`assignment_1` (`ssid`, `rssi`,`tilt`, `humidity`, `temperature`, `light`, `alarm`, `warning`) VALUES ('%s',%d,%d, %f, %f, %d, %d, %d)";  //Template for the standard query to the DB

//light Sensor cfg
static unsigned int lightSensorValue;
int LightRead_count = 1;

// Web Server config
ESP8266WebServer server(80);  // HTTP server on port 80
// Declare a WiFiClient object to handle WiFi connections and an ESP8266WebServer object to handle HTTP requests on port 80.


//Variables for the DHT Humidity and Temperature sensor.
unsigned long previousMillisDHT = millis();
float h = 0;  //DHT sensor humidity
float t = 0;  //DHT sensor temperature
float hic;    //heat index in Celsius

// Variable for TILT sensor
byte tilt = digitalRead(TILT);  // read the SW-520D state

// LCD setup
LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES);  // display object
unsigned long lastLcdRefresh = millis();                            // init with millis at first call, numero di millisecondi da quando è stato alimentato il processore/ la scheda


//Variable Alarm
boolean Alarm = true;  //Button to deactivate the alarm. If True the alarm is active, and includes all the alerts from the various sensors. Deactivating it deactivates everyone

// DHT setup
unsigned long buzzerStartTime = 0;  // Variabile per memorizzare il tempo di inizio del suono del buzzer
bool buzzerOn = false;              // Variabile per tenere traccia dello stato del buzzerù
int DhtRead_count = 1;

// Initialize DHT sensor
DHT dht = DHT(DHTPIN, DHTTYPE);

// TILT sensor setup
unsigned long lastDebounceTime_TILT = 0;

//MQTT variables
MQTTClient mqttClient(MQTT_BUFFER_SIZE);  // handles the MQTT communication protocol
WiFiClient networkClient;                 // handles the network connection to the MQTT broker

byte flame[3] = { 0, 0, 0 };
byte movement[3] = { 0, 0, 0 };
bool alarm_storage[3] = { true, true, true};
int waterLevel[3] = { 0, 0, 0 };
bool manualIrrigation[3] = { false, false, false };
bool LWT_slave[3] = { true, true, true };
String movement_timestamp[3] = { "No Movement Detected Yet", "No Movement Detected Yet", "No Movement Detected Yet" };
String MAC_slave[3] = { "No Mac Slave connected", "No Mac Slave connected", "No Mac Slave connected" };
String MAC_master = "error in reading mac address";
String MAC_slave_Type[3] = { "-", "-", "-" };


// WEATHER API (refer to https://openweathermap.org/current)
const char weather_server[] = "api.openweathermap.org";
const char weather_query[] = "GET /data/2.5/weather?q=%s,%s&units=metric&APPID=%s";

String locationName;
String countryName;
float tempAPI;
float humidityAPI;
String weatherAPI;
String weatherDescriptionAPI;
float sunrise;
float sunset;
float tempMinAPI;
float tempMaxAPI;
float windSpeed;

// LOCATION API Variables
String currentCity;
String currentCountryCode;
String currentCountry;
String currentRegionName;
String currentZip;
String currentTimezone;

//Email 
String userEmail = "Nessuna email inserita";  // Variabile per memorizzare la stringa dell'utente




// Handler to set new threshold values
void handleSetThresholds() {
  if (server.hasArg("temp") && server.hasArg("hum") && server.hasArg("light")) {
    int newTemp = server.arg("temp").toInt();
    int newHum = server.arg("hum").toInt();
    int newLight = server.arg("light").toInt();

    if (newTemp >= -50 && newTemp <= 150) SogliaTemperatura = newTemp;  //values must be inside range of accepted values
    if (newHum >= 0 && newHum <= 100) SogliaUmidita = newHum;
    if (newLight >= 0 && newLight <= 1024) SogliaLuce = newLight;

    server.send(200, "text/plain", "Thresholds updated successfully");
  } else {
    server.send(400, "text/plain", "Invalid arguments");
  }
}


void setup() {

  // set Alarm LED pins as outputs
  pinMode(LED_GREEN_ALARM, OUTPUT);  // Set the green led pin as output
  pinMode(LED_RED_ALARM, OUTPUT);    // Set the red led pin as output

  // turn led off
  digitalWrite(LED_GREEN_ALARM, HIGH);
  digitalWrite(LED_RED_ALARM, HIGH);

  // set LED pin as outputs
  pinMode(LED_FOTORESISTOR, OUTPUT);


  // turn led off
  digitalWrite(LED_FOTORESISTOR, HIGH);

  // set Web Server
  //Match different HTTP requests to specific functions. When an HTTP request arrives on the root (/),
  //the handle_root function is called. If the request is /ON, handle_ledon is called,
  //while for /OFF handle_ledoff is called.
  //If no matching path is found, handle_NotFound is called.
  server.on("/", handle_root);
  server.on("/ON", handle_ledon);
  server.on("/OFF", handle_ledoff);
  server.onNotFound(handle_NotFound);
  server.on("/toggle_irrigation", HTTP_POST, handleToggleIrrigation);
  //alarm storage button showed in web
  server.on("/AlarmStorageON", handle_AlarmStorageON);    // Handler for enabling alarm storage
  server.on("/AlarmStorageOFF", handle_AlarmStorageOFF);  // Handler for disabling alarm storage
  //email insertion
    server.on("/submitEmail", handleSubmitEmail);
  //start the web server on port 80
  server.begin();
  Serial.println(F("HTTP server started"));

  server.on("/setThresholds", handleSetThresholds);  // Handler for setting new thresholds

  // set LCD
  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(F("LCD found."));
    lcd.begin(DISPLAY_CHARS, 2);  // initialize the lcd

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

  // setup MQTT
  mqttClient.begin(MQTT_BROKERIP, 1883, networkClient);  // setup communication with MQTT broker
  mqttClient.onMessage(mqttMessageReceived);             // callback on message received from MQTT broker

  Serial.begin(115200);
  Serial.println(F("\n\nSetup completed.\n\n"));

  MAC_master = WiFi.macAddress();
  Serial.print("Device MAC Address: ");
  Serial.println(WiFi.macAddress());
}




void loop() {
  static bool firstIteration_Values = true;  // variable for the first read of the sensors, to be displayed

  //////// wifi section ///////
  wifiConnection();

  //////// WEB SERVER & MQTT section ///////
  server.handleClient();

  connectToMQTTBroker();  // connect to MQTT broker (if not already connected)
  mqttClient.loop();      // MQTT client loop

  //////// first reads section, it compile once and get the values from all sensors. it is used to enable the reading for longer intervals, as for the first iteraction the values are already measured ///////
  if (firstIteration_Values) {
    automatic_light_sleep();
    //MQTT_topics_writing();
    // Ottenimento geolocalizzazione all'avvio
    getGeoLocation();

    // Se la geolocalizzazione fallisce, usa la città predefinita
    if (currentCity.length() == 0 || currentCountry.length() == 0) {
      Serial.println(F("City & Country initialization failed, using default city."));
      currentCity = WEATHER_CITY;
      currentCountry = WEATHER_COUNTRY;
    }
    // true per fare eseguire il metodo senza aspettare la temporizzazione con il millis in modo da recuperare i valori da mostrare nel sito web
    
    storeCurrentWeather(true);

    First_Values();

    //initLCD is executed at the first execution of the loop(), after which refreshLCD() is executed every 1200000 ms
    initLCD();
    firstIteration_Values = false;
  }

  //////// WiFi strenght section ///////
  readWifiStatus();

  //////// LIGHT SENSOR section ///////
  readLightSensor();

  //////// DHT SENSOR section ///////
  if (millis() - previousMillisDHT >= intervalDHT) {  // controlla se è passato l'intervallo
    DHT_read();                                       //Aggiornamento valori DHT
  }

  //Section that enables the Buzzer
  if ((h > SogliaUmidita || t > SogliaTemperatura) && Alarm == true) {
    // Turn on the buzzer if the humidity exceeds SogliaUmidita or the temperature exceeds SogliaTemperatura degrees
    digitalWrite(BUZZER_DHT, LOW);  // turn on the buzzer
  }

  if ((digitalRead(BUZZER_DHT) == LOW && h < SogliaUmidita && t < SogliaTemperatura) || Alarm == false) {
    // Turn off the buzzer if it is on and the above conditions are not met
    digitalWrite(BUZZER_DHT, HIGH);  // turn off the buzzer
  }
  //////// TILT SENSOR section ///////
  TILT_read();  // Continuous update of the tilt sensor status

  //as soon as the loop is started refreshLCD() is not executed immediately, as soon as a few seconds have passed it starts to be executed
  refreshLCD();

  //Function that manages the alarm button
  buttonAlarm();

  //Updating the alarm status LED
  ledAlarm();

  //Write to SQL DB
  if(WriteMultiToDB(ssid, signalStrenght, tilt, h, t, lightSensorValue, Alarm)){
    // write on MySQL table if connection works
    // dopo aver eseguito la scrittura sul db (operazione dispendiosa in tempo rispetto alla durata del loop) riparte l'esecuzione dall'inizio del loop per fare in modo di eseguire "server.handleClient();" e "mqttClient.loop();"
    return;
  }  


  // prima di rispondere alle richieste del web server salva le informazioni meteo da mostrare sul sito
  if(storeCurrentWeather(false)){
    // dopo aver eseguito la chiamata ad API (operazione dispendiosa in tempo rispetto alla durata del loop) riparte l'esecuzione dall'inizio del loop per fare in modo di eseguire "server.handleClient();" e "mqttClient.loop();"
    return;
  }

  //delay usato per il risparmio energetico con light sleep automatic
  delay(50);
}






void automatic_light_sleep() {
  Serial.println(F("\n\n============ Energy Saving Settings START ============"));
  Serial.println(F("Configuring Automatic Light Sleep..."));
  Serial.flush();
  // WiFi.setSleepMode(WIFI_LIGHT_SLEEP);   // automatic Light Sleep, wakes up at every Delivery Traffic Indication Message (DTIM) configured by the access point
  // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html#wifi-power-management-dtim
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP, 5);  // automartic Light Sleep, wakes every 5*100ms to receive DTIMs (with intervals >3 it may be
                                           // difficult to establish and maintain a connection)
                                           // the second parameter is the DTIMs listen interval and ranges from 1 (1*100ms = 100ms, less energy saving)
                                           // to (10*100ms = 1s, max energy saving), 0 is a special value (and also the default) that forces the chip
                                           // to obey the access-point-defined DTIM interval.
  // wifi_set_sleep_type(LIGHT_SLEEP_T);
  // esp_yield();
  // wifi_fpm_open();
  // esp_yield();
  Serial.println(F("Automatic Light Sleep enabled"));
  Serial.println(F("============ Energy Saving Settings END ============ "));
}

void buttonAlarm() {
  if (isButtonPressed() == true) {  // Button pressed
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

boolean isButtonPressed() {
  static byte lastState = digitalRead(BUTTON);  // the previous reading from the input pin

  for (byte count = 0; count < BUTTON_DEBOUNCE_DELAY; count++) {
    if (digitalRead(BUTTON) == lastState) return false;
    delay(1);
  }

  lastState = !lastState;
  return lastState == HIGH ? false : true;
}

void displaySystemOn() {
  // clear display and write the message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Monitoring ON");
  lcd.setCursor(0, 1);
  lcd.print("System active");

  lastLcdRefresh = millis();
}

void displaySystemOff() {
  // clear display and write the message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Monitoring OFF");
  lcd.setCursor(0, 1);
  lcd.print("System inactive");

  lastLcdRefresh = millis();
}

void ledAlarm() {
  if (Alarm) {                            // If the alarm is active
    digitalWrite(LED_GREEN_ALARM, HIGH);  // Turns on the green LED
    digitalWrite(LED_RED_ALARM, LOW);     // Turns off the red LED
  } else {                                // If the alarm is not active
    digitalWrite(LED_GREEN_ALARM, LOW);   // Turns off the green LED
    digitalWrite(LED_RED_ALARM, HIGH);    // Turns on the red LED
  }
}

void initLCD() {
  lcd.setBacklight(255);          // Set the backlight to maximum
  lcd.home();                     // Move the cursor to 0.0
  lcd.clear();                    // Clear the text
  lcd.print("System starting");   // Show startup text on first line
  lcd.setCursor(0, 1);            // Move the cursor to the second line
  for (int i = 0; i < 16; i++) {  // Print the loading bar
    lcd.print(".");
    delay(100);  // Add a small delay to make the movement of the bar visible
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
  if (tilt == HIGH) {
    lcd.print("Open ");
  } else {
    lcd.print("Close");
  }
}

void refreshLCD() {

  // Read LIGHT Sensor every LIGHT_READ_INTERVAL milliseconds
  if (millis() - lastLcdRefresh > LCD_REFRESH_INTERVAL) {  //if the current time minus the past instant in which I changed the LED state is greater than 500 then I change the LED state

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
    if (tilt == HIGH) {
      lcd.print("Open ");
    } else {
      lcd.print("Close");
    }


    lastLcdRefresh = millis();  // update last execution millis
  }
}

void displaySlaveOff() {  //funciotn that print a warning message when LWT message has arrived
  // clear display and write the message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Slave System OFF");
  lcd.setCursor(0, 1);
  lcd.print("System inactive");

  lastLcdRefresh = millis();
}

void displaySlaveOn() {
  // clear display and write the message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Slave System ON");
  lcd.setCursor(0, 1);
  lcd.print("System active");

  lastLcdRefresh = millis();
}

void readLightSensor() {
  static unsigned long lastLightRead = millis();  // init with millis at first call, numero di millisecondi da quando è stato alimentato il processore/ la scheda

  // Read LIGHT Sensor every LIGHT_READ_INTERVAL milliseconds
  if (millis() - lastLightRead > LIGHT_READ_INTERVAL) {  //se il tempo attuale meno l'istante passato in cui ho cambiato lo stato del led è maggiore a 500 allora cambio stato al led

    // for brightness reading we plan to take 3 measurements (each measurement taken every LIGHT_READ_INTERVAL), calculate the average and then save it to the database every DB_WRITE_INTERVAL (use 1 hour for real use)
    // for real use defined to measure brightness every 20 minutes and made the third measurement, every 60 minutes write the average to the db.
    if (LightRead_count < 3) {
      lightSensorValue = (lightSensorValue + analogRead(PHOTORESISTOR)) / 2;

      LightRead_count++;
    } else {
      lightSensorValue = analogRead(PHOTORESISTOR);  // read analog value (range 0-1023)

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
  if (lightSensorValue <= SogliaLuce && Alarm == true) {  // high brightness
    digitalWrite(LED_FOTORESISTOR, LOW);                  // LED_FOTORESISTOR on
  } else {                                                // low brightness
    digitalWrite(LED_FOTORESISTOR, HIGH);                 // LED_FOTORESISTOR off
  }
}

void DHT_read() {                //Updating DHT values
  previousMillisDHT = millis();  // save the current time as the last measurement time

  // for temperature and humidity reading we plan to take 3 measurements (each measurement taken every intervalDHT), calculate the average and then save it to the database every DB_WRITE_INTERVAL (use 1 hour for real use)
  // for real use you defined to measure temperature and humidity every 20 minutes and made the third measurement, every 60 minutes write the average to the db.
  if (DhtRead_count < 3) {

    // Read humidity and temperature
    h = (h + dht.readHumidity()) / 2;

    t = (t + dht.readTemperature()) / 2;

    if (isnan(h) || isnan(t)) {  // check if the read failed
      Serial.println(F("\nUnable to read from DHT sensor!"));
      return;
    }

    hic = (hic + dht.computeHeatIndex(t, h, false)) / 2;

    DhtRead_count++;

  } else {
    h = dht.readHumidity();     // humidity in percentage, between 20 and 80% (accuracy ±5%)
    t = dht.readTemperature();  // temperature in Celsius, between 0 and 50°C (accuracy ±2°C)
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
  Serial.print(F("°C Perceived temperature: "));  // the temperature perceived by humans (considers humidity)
  Serial.print(hic);
  Serial.println(F("°C"));
}


void wifiConnection() {
  // connect to WiFi (if not already connected)
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("\nConnecting to SSID: "));
    Serial.println(SECRET_SSID);

#ifdef IP                                   //if constant IP is defined then assign static IP, DNS, gateway and subnet (parameters defined in the secrets.h file)
    WiFi.config(ip, dns, gateway, subnet);  // by default network is configured using DHCP
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
  static unsigned long lastWifistatus = millis();  // init with millis at first call

  // Read Wifi status every WIFI_STATUS_INTERVAL milliseconds
  if (millis() - lastWifistatus > WIFI_STATUS_INTERVAL) {  //if the current time minus the past instant in which I changed the LED state is greater than 500 then I change the LED state

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

    lastWifistatus = millis();  // update last execution millis
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
      digitalWrite(LED_TILT, HIGH);  // Turn on the LED_TILT
    } else {
      digitalWrite(LED_TILT, LOW);  // Turn off the LED_TILT
    }
  }
}


void First_Values() {
  // humidity and temperature
  // Read humidity and temperature
  h = dht.readHumidity();     // humidity in percentage, between 20 and 80% (accuracy ±5%)
  t = dht.readTemperature();  // temperature in Celsius, between 0 and 50°C (accuracy ±2°C)

  if (isnan(h) || isnan(t)) {  // verify if reading failed
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
  Serial.print(F("°C  Temperatura percepita: "));  // percepted temperature
  Serial.print(hic);
  Serial.println(F("°C"));

  //TILT
  tilt = digitalRead(TILT);

  //signal strenght
  signalStrenght = WiFi.RSSI();
  Serial.print(F("First Signal strength (RSSI): "));
  Serial.print(signalStrenght);
  Serial.println(" dBm\n");

  Serial.print(F("\nLettura 1 sensore Luminosità\n"));
  lightSensorValue = analogRead(PHOTORESISTOR);  // read analog value (range 0-1023)
  Serial.print(F("Light sensor value: "));
  Serial.println(lightSensorValue);
}


bool storeCurrentWeather(bool firstIteration) {
  static unsigned long lastweatherApiRead = millis();

  if (millis() - lastweatherApiRead > WEATHER_API_READ_INTERVAL || firstIteration) {

    // Current weather api documentation at: https://openweathermap.org/current
    Serial.println(F("\n=== Current weather ==="));

    // call API for current weather
    if (client.connect(weather_server, 80)) {
      char request[100];
      sprintf(request, weather_query, currentCity, currentCountry, WEATHER_API_KEY);
      client.println(request);
      client.println(F("Host: api.openweathermap.org"));
      client.println(F("User-Agent: ArduinoWiFi/1.1"));
      client.println(F("Connection: close"));
      client.println();
    } else {
      Serial.println(F("Connection to api.openweathermap.org failed!\n"));
    }

    while (client.connected() && !client.available()) delay(1);  // wait for data
    String result;
    while (client.connected() || client.available()) {  // read data
      char c = client.read();
      result = result + c;
    }

    client.stop();  // end communication
    // Serial.println(result);  // print JSON

    char jsonArray[result.length() + 1];
    result.toCharArray(jsonArray, sizeof(jsonArray));
    jsonArray[result.length() + 1] = '\0';
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonArray);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return false;
    }


    locationName = doc["name"].as<String>();
    countryName = doc["sys"]["country"].as<String>();
    tempAPI = (float)doc["main"]["temp"];
    humidityAPI = (float)doc["main"]["humidity"];
    weatherAPI = doc["weather"][0]["main"].as<String>();
    weatherDescriptionAPI = doc["weather"][0]["description"].as<String>();
    sunrise = (float)doc["sys"]["sunrise"];
    sunset = (float)doc["sys"]["sunset"];
    tempMinAPI = (float)doc["main"]["temp_min"];
    tempMaxAPI = (float)doc["main"]["temp_max"];
    windSpeed = (float)doc["wind"]["speed"];


    Serial.print(F("Location: "));
    Serial.println(locationName);
    Serial.print(F("Country: "));
    Serial.println(countryName);
    Serial.print(F("Temperature (°C): "));
    Serial.println(tempAPI);
    Serial.print(F("Humidity (%): "));
    Serial.println(humidityAPI);
    Serial.print(F("Weather: "));
    Serial.println(weatherAPI);
    Serial.print(F("Weather description: "));
    Serial.println(weatherDescriptionAPI);
    Serial.print(F("Sunrise (UNIX timestamp): "));
    Serial.println(sunrise);
    Serial.print(F("Sunset (UNIX timestamp): "));
    Serial.println(sunset);
    Serial.print(F("Temperature min. (°C): "));
    Serial.println(tempMinAPI);
    Serial.print(F("Temperature max. (°C): "));
    Serial.println(tempMaxAPI);
    Serial.print(F("Wind speed (m/s): "));
    Serial.println(windSpeed);

    Serial.println(F("==============================\n"));

    lastweatherApiRead = millis();
    return true;
  }
  return false;
}

// Funzione per ottenere la geolocalizzazione IP
void getGeoLocation() {
  Serial.println(F("\n=== Current IP Location ==="));

  if (WiFi.status() == WL_CONNECTED) {  // Controlla se sei connesso al WiFi
    IPAddress ip = WiFi.localIP();      // Ottieni l'indirizzo IP locale
    String ipStr = ip.toString();

    HTTPClient http;
    String url = "http://ip-api.com/json/" + ipStr;

    http.begin(client, url);    // URL dell'API di geolocalizzazione IP
    int httpCode = http.GET();  // Fai una richiesta GET

    if (httpCode > 0) {  // Controlla se la richiesta ha avuto successo
      String payload = http.getString();
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }

      // Estrai le informazioni dal JSON
      currentCity = doc["city"].as<String>();
      currentCountryCode = doc["countryCode"].as<String>();
      currentCountry = doc["country"].as<String>();
      currentRegionName = doc["regionName"].as<String>();
      currentZip = doc["zip"].as<String>();
      currentTimezone = doc["timezone"].as<String>();

      Serial.print("Current City: ");
      Serial.println(currentCity);
      Serial.print("Current Country Code: ");
      Serial.println(currentCountryCode);
      Serial.print("Current Country: ");
      Serial.println(currentCountry);
      Serial.print("Current Region: ");
      Serial.println(currentRegionName);
      Serial.print("Current Zip: ");
      Serial.println(currentZip);
      Serial.print("Current Timezone: ");
      Serial.println(currentTimezone);
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpCode);
    }

    http.end();  // Chiudi la connessione
  } else {
    Serial.println("WiFi not connected");
  }
  Serial.println(F("==============================\n"));
}
