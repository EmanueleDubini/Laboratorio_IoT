/*
 * ****************************************************************************
 *  University of Milano-Bicocca
 *  DISCo - Department of Informatics, Systems and Communication
 *  Viale Sarca 336, 20126 Milano, Italy
 *
 * ****************************************************************************
 *
 * Third Assignment - Iot Laboratory - NodeMCU slave device
 *
 */

//TODO aggiungere autoconnect


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


// include WiFi library
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <DNSServer.h>

// include LCD libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// MQTT libraries
#include <ArduinoJson.h>
#include <MQTT.h>

// MySQL libraries //TODO togliere se non usato
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

// include DHT library
#include <DHT.h>  //Humidity sensor

//Login and connection informations
#include "secrets.h"


// LCD setup
#define DISPLAY_CHARS 16   // number of characters on a line
#define DISPLAY_LINES 2    // number of display lines
#define DISPLAY_ADDR 0x27  // display address on I2C bus

#define LCD_REFRESH_INTERVAL 5000  // refresh LCD display each 5000 ms
#define intervalDHT 5000           // interval between DHT sensor measurements in milliseconds
#define WIFI_STATUS_INTERVAL 5000  // read WIFI status each 5000 ms
#define WATER_READ_INTERVAL 5000   // read water level each 5000 ms
#define MQTT_WRITE_INTERVAL 5000   // write sensor data on mqtt each 5000 ms

#define MQTT_BUFFER_SIZE 512  // the maximum size for packets being published and received

// PIN definition section
#define LED_PIN D0                // PWM-capable LED pin
#define BUTTON D3                 // Button pin, eg. D1 is GPIO5 and has an optional internal pull-up
#define DHTPIN D4                 // sensor I/O pin
#define DHTTYPE DHT11             // sensor type DHT 11
#define BUTTON_DEBOUNCE_DELAY 20  // Button debounce time in ms

// Water level sensor section
#define WATER_LEVEL_PIN A0  // Analog pin for water level sensor
bool noWater = false;
int sogliaLivelloAcqua = 20;

int ledBrightness = 0;  // Initial LED brightness
int waterLevel = 0;     // Water level

// Threshold dht global variables
int SogliaTemperatura = 35;  // Default temperature threshold
int SogliaUmidita = 75;      // Default humidity threshold


//Variables for the DHT Humidity and Temperature sensor.
float h = 0;  //DHT sensor humidity
float t = 0;  //DHT sensor temperature
float hic;    //heat index in Celsius

// Initialize DHT sensor
DHT dht = DHT(DHTPIN, DHTTYPE);



//LCD Variables
LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES);  // Display object
bool manualIrrigationMode = false;
bool autoIrrigationMode = false;
unsigned long lastLcdRefresh = millis();  // Init with millis at first call



// MQTT data
MQTTClient mqttClient(MQTT_BUFFER_SIZE);  // handles the MQTT communication protocol
WiFiClient networkClient;                 // handles the network connection to the MQTT broker
String MQTT_TOPIC_CONTROL;                // topic to get information about MQTT connections

String waterLevelTopic;
String manualIrrigationTopic;
String masterComunicationsTopic;
boolean topicPathUpdated = false;

// MySQL server cfg TODO se poi non usato togliere
WiFiClient client;  // WiFi client class that offers methods to connect to WiFi

IPAddress MYSQL_IP;
char mysql_user[50];
char mysql_password[50];
MySQL_Connection conn((Client *)&client);


String MAC_device = "error in reading mac address";
int signalStrength;


//TODO sistemare variabili
// Variables for fade effect
int fadeDirection = 1;  // 1 for increasing brightness, -1 for decreasing
int fadeValue = 0;      // Current brightness level
unsigned long lastFadeUpdate = 0;  // Last time the LED brightness was updated
const unsigned long fadeInterval = 10;  // Interval for fade effect update

// WiFiManager
WiFiManager wifiManager;

void setup() {
  // Rotary encoder section
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(WATER_LEVEL_PIN, INPUT);

  // Turn LED off
  analogWrite(LED_PIN,fadeValue);

  // DHT sensor
  dht.begin();

  // Set LCD
  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(F("LCD found."));
    lcd.begin(DISPLAY_CHARS, 2);  // Initialize the LCD
  } else {
    Serial.print(F("LCD not found. Error "));
    Serial.println(error);
    Serial.println(F("Check connections and configuration. Reset to try again!"));
    while (true)
      delay(1);
  }

  Serial.begin(115200);
  Serial.println(F("\n\nSetup completed.\n\n"));


  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();  //si resettano tutti i parametri memorizzati dalla libreria ssid e ps

  // set custom ip for portal
  // wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("904078_EMA");  //crea un access point con nome della rete wifi specificato per parametro
  // or use this for auto generated name ESP + ChipID
  // wifiManager.autoConnect();

  // if you get here you have connected to the WiFi
  Serial.println("Connected.");

  // Print and save SSID value
  Serial.print("Connected to SSID: ");
  Serial.println(WiFi.SSID());

  // set WIFI
  WiFi.mode(WIFI_STA);


  MAC_device = WiFi.macAddress();
  Serial.print("Device MAC Address: ");
  Serial.println(MAC_device);

  mqttClient.begin(MQTT_BROKERIP, 1883, networkClient);  // setup communication with MQTT broker
  mqttClient.onMessage(mqttMessageReceived);             // callback on message received from MQTT broker
  
  automatic_light_sleep();  // enable automatic light sleep
}

void loop() {
  static bool firstIteration_Values = true;  // Variable for the first read of the sensors, to be displayed

  // WiFi section
  //!!!wifiConnection();

  if (firstIteration_Values) {
    // TODO leggere valori dai sensori da mostrare sullo schermo, completare funzione
    firstValues();

    // initLCD is executed at the first execution of the loop(), after which refreshLCD() is executed every 1200000 ms
    initLCD();
    firstIteration_Values = false;
  }

  connectToMQTTBroker();  // connect to MQTT broker (if not already connected)
  mqttClient.loop();      // MQTT client

  // WiFi strength section
  readWifiStatus();

  // Water level reading
  readWaterLevel();

  // DHT reading
  readDHT();

  // manual irrigation reading
  readManualIrrigation();

  // Refresh LCD
  refreshLCD(false);

  MQTTreadings(false);

  if (h >= 70 && !manualIrrigationMode && !noWater) {
    autoIrrigationMode = true;

    digitalWrite(LED_PIN, HIGH);

  } else {
    autoIrrigationMode = false;
  }

  delay(250);  //implementato un delay di 250 millisecondi per il risparmio energetico
}

void readWaterLevel() {
  static unsigned long lastWaterRead = millis();  // Init with millis at first call

  if (millis() - lastWaterRead > WATER_READ_INTERVAL) {
    // Read the analog value from the potentiometer
    int analogValue = analogRead(WATER_LEVEL_PIN);

    // Convert the analog value to a percentage (0 to 100)
    waterLevel = map(analogValue, 0, 1023, 0, 100);

    if (waterLevel >= sogliaLivelloAcqua) {
      noWater = false;
    } else {
      noWater = true;
    }

    // Print the water level to the Serial Monitor
    Serial.print(F("Water Level: "));
    Serial.print(waterLevel);
    Serial.println(F("%"));

    lastWaterRead = millis();  // Update last execution millis
  }
}

void readDHT() {
  static unsigned long lastDHTRead = millis();  // Init with millis at first call

  if (millis() - lastDHTRead > intervalDHT) {
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
    Serial.print(F("\nLettura sensore DHT\n"));
    Serial.print(F("Umidità: "));
    Serial.print(h);
    Serial.print(F("%  Temperatura: "));
    Serial.print(t);
    Serial.print(F("°C  Temperatura apparente: "));  // percepted temperature
    Serial.print(hic);
    Serial.println(F("°C"));

    lastDHTRead = millis();  // Update last execution millis
  }
}


boolean isButtonPressed() {
  static byte lastState = digitalRead(BUTTON);  // The previous reading from the input pin

  for (byte count = 0; count < BUTTON_DEBOUNCE_DELAY; count++) {
    if (digitalRead(BUTTON) == lastState) return false;
    delay(1);
  }

  lastState = !lastState;
  return lastState == HIGH ? false : true;
}

void readManualIrrigation() {
  byte buttonVal = digitalRead(BUTTON);
  if (isButtonPressed() == true) {
    Serial.println(F("Button: PRESSED"));
    if (!noWater) {
      manualIrrigationMode = !manualIrrigationMode;
      Serial.println("Irrigazione Manuale= " + String(manualIrrigationMode));
      refreshLCD(true);
    } else {
      showNoWaterMessageLCD();
    }
  }

if (manualIrrigationMode) {
// Update fade effect
    /*unsigned long currentMillis = millis();
    if (currentMillis - lastFadeUpdate >= fadeInterval) {
      lastFadeUpdate = currentMillis;
      fadeValue += fadeDirection;
      if (fadeValue <= 0) {
        fadeValue = 0;
        fadeDirection = 1;
      } else if (fadeValue >= 255) {
        fadeValue = 255;
        fadeDirection = -1;
      }
      analogWrite(LED_PIN, fadeValue);
    }*/

    digitalWrite(LED_PIN, HIGH);
  } else if (!manualIrrigationMode && !autoIrrigationMode) {
    // Ensure the LED is off when manual irrigation mode is disabled
    //analogWrite(LED_PIN, 0);
    digitalWrite(LED_PIN, LOW);
  }
}


void firstValues() {
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
  Serial.print(F("\nLettura sensore DHT\n"));
  Serial.print(F("Umidità: "));
  Serial.print(h);
  Serial.print(F("%  Temperatura: "));
  Serial.print(t);
  Serial.print(F("°C  Temperatura percepita: "));  // percepted temperature
  Serial.print(hic);
  Serial.println(F("°C"));


  // Signal strength
  signalStrength = WiFi.RSSI();
  Serial.print(F("First Signal strength (RSSI): "));
  Serial.print(signalStrength);
  Serial.println(" dBm\n");

  int analogValue = analogRead(WATER_LEVEL_PIN);
  // Convert the analog value to a percentage (0 to 100)
  waterLevel = map(analogValue, 0, 1023, 0, 100);
  // Print the water level to the Serial Monitor
  Serial.print(F("Water Level: "));
  Serial.print(waterLevel);
  Serial.println(F("%"));
}

/*void wifiConnection() {
  // Connect to WiFi (if not already connected)
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("\nConnecting to SSID: "));
    Serial.println(SECRET_SSID);

#ifdef IP                              // If there is a defined IP in secrets.h
    WiFi.config(IP, Gateway, Subnet);  // Assign the parameters
    Serial.print("IP statico: ");      // Show
    Serial.println(IP);
#endif

    WiFi.begin(SECRET_SSID, SECRET_PASS);  // Connect to WiFi

    while (WiFi.status() != WL_CONNECTED) {  // Check if the module is connected to the Wi-Fi network
      delay(500);                            // Every 500 ms
      Serial.print(".");                     // Print a point to show the waiting for connection
    }

    Serial.println(F("\n\nConnected.\n"));
    Serial.print("Device IP: ");
    Serial.println(WiFi.localIP());
  }
}*/

void readWifiStatus() {
  static unsigned long lastWifiRead = millis();  // Init with millis at first call

  if (millis() - lastWifiRead > WIFI_STATUS_INTERVAL) {
    // Read the Wi-Fi signal strength
    signalStrength = WiFi.RSSI();
    Serial.print(F("Signal strength (RSSI): "));
    Serial.print(signalStrength);
    Serial.println(F(" dBm"));

    lastWifiRead = millis();  // Update last execution millis
  }
}

void automatic_light_sleep() {
  Serial.println(F("Configuring Automatic Light Sleep..."));
  Serial.flush();
  // WiFi.setSleepMode(WIFI_LIGHT_SLEEP);   // automatic Light Sleep, wakes up at every Delivery Traffic Indication Message (DTIM) configured by the access point
  // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html#wifi-power-management-dtim
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP, 2);  // automartic Light Sleep, wakes every 2*100ms to receive DTIMs (with intervals >3 it may be
                                           // difficult to establish and maintain a connection)
                                           // the second parameter is the DTIMs listen interval and ranges from 1 (1*100ms = 100ms, less energy saving)
                                           // to (10*100ms = 1s, max energy saving), 0 is a special value (and also the default) that forces the chip
                                           // to obey the access-point-defined DTIM interval.
  // wifi_set_sleep_type(LIGHT_SLEEP_T);
  // esp_yield();
  // wifi_fpm_open();
  // esp_yield();
  Serial.println(F("Automatic Light Sleep enabled"));
}
