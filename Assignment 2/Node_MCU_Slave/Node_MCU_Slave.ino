/*
 * ****************************************************************************
 *  University of Milano-Bicocca
 *  DISCo - Department of Informatics, Systems and Communication
 *  Viale Sarca 336, 20126 Milano, Italy
 *
 *  Copyright © 2019-2024 by:
 *    Davide Marelli   - davide.marelli@unimib.it
 *    Paolo Napoletano - paolo.napoletano@unimib.it
 * ****************************************************************************
 *
 * Second Assignment - Iot Laboratory - NodeMCU slave device
 *
 * Components:
 *   1x NodeMCU 1.0 (ESP8266)
 *
 * Libraries:
 *   - requires 'WiFiManager' library by tzapu, install it from the Arduino IDE library manager
 *       documentation: https://github.com/tzapu/WiFiManager
 *
 *   - NTPClient by Fabrice weinberg
 *
 */

//Come valore normale il sensore di mmovimento ha HIGH, se rileva movimento va a LOW
// Come valore normale il sensore di fiamma ha valore LOW, se rileva fiamma va a HIGH;

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

// NTP libraries
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

// MySQL libraries
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

// MQTT libraries
#include <ArduinoJson.h>
#include <MQTT.h>

//todo da rimuovere
#include "secrets.h"




#define FLAME_DETECTOR D1
#define FLAME_LED D2
#define MOTION_DETECTOR D5
#define MOTION_LED D4

// Button setup
// Button for the activation e deactivation of the alarm
#define BUTTON D6                 // button pin, eg. D1 is GPIO5 and has an optional internal pull-up
#define BUTTON_DEBOUNCE_DELAY 20  // button debounce time in ms

//LED green and red for Alarm
#define LED_GREEN_ALARM D7  // green LED for when alarm is active
#define LED_RED_ALARM D8

#define FLAME_STATUS_INTERVAL 5000  // read flame sensor each 5000 ms
#define MOTION_DEBOUNCE_DELAY 150    // Debounce delay for motion sensor in milliseconds

#define DB_WRITE_INTERVAL 10000  // cooldown period after writing to the database, in milliseconds
#define WIFI_STATUS_INTERVAL 5000  // read WIFI status each 5000 ms
#define MQTT_WRITE_INTERVAL 5000  // write sensor data on mqtt each 5000 ms

#define MQTT_BUFFER_SIZE 512               // the maximum size for packets being published and received





// Variables for debounce logic
byte lastMotionState = HIGH;         // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled



// MySQL server cfg
WiFiClient client;  //wifi client class that offers methods to connect to wifi

IPAddress MYSQL_IP;
char mysql_user[50];
char mysql_password[50];
MySQL_Connection conn((Client *)&client);

char query[256];  //caratteri per query sql
char INSERT_DATA[] = "INSERT INTO `%s`.`A2_slaveMCU` (`ssid`, `rssi`, `ir_obstacle`, `flame`, `alarm`, `message`) VALUES ('%s', %d, %d, %d, %d, '%s')";



// MQTT data
MQTTClient mqttClient(MQTT_BUFFER_SIZE);  // handles the MQTT communication protocol
WiFiClient networkClient;                 // handles the network connection to the MQTT broker
String MQTT_TOPIC_CONTROL;                 // topic to get information about MQTT connections

String flameDetectionTopic;
String motionDetectionTopic;
String alarmTopic;  // Variable to store the topic for alarm
String masterComunicationsTopic;
boolean topicPathUpdated = false;


//Variable Alarm
boolean alarm = true;  //Button to deactivate the alarm. If True the alarm is active
byte flameValue = LOW;
byte motionValue = HIGH;  // Initial state for motion sensor

String ssid;
int signalStrenght;


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // Sincronizzazione ogni minuto


void setup() {

  pinMode(FLAME_DETECTOR, INPUT);
  pinMode(FLAME_LED, OUTPUT);

  pinMode(MOTION_DETECTOR, INPUT);
  pinMode(MOTION_LED, OUTPUT);

  // set BUTTON pin as input with pull-up
  pinMode(BUTTON, INPUT_PULLUP);

  // set Alarm LED pins as outputs
  pinMode(LED_GREEN_ALARM, OUTPUT);  // Set the green led pin as output
  pinMode(LED_RED_ALARM, OUTPUT);    // Set the red led pin as output

  digitalWrite(FLAME_LED, HIGH);
  digitalWrite(MOTION_LED, HIGH);

  // turn led off
  digitalWrite(LED_GREEN_ALARM, LOW);
  digitalWrite(LED_RED_ALARM, LOW);


  Serial.begin(115200);
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

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
  ssid = WiFi.SSID();
  Serial.print("Connected to SSID: ");
  Serial.println(ssid);

  // set WIFI
  WiFi.mode(WIFI_STA);

  mqttClient.begin(MQTT_BROKERIP, 1883, networkClient);  // setup communication with MQTT broker
  mqttClient.onMessage(mqttMessageReceived);             // callback on message received from MQTT broker

  timeClient.begin();
  timeClient.update();
}

void loop() {

  connectToMQTTBroker();  // connect to MQTT broker (if not already connected)
  mqttClient.loop();      // MQTT client

  //todo inserire che se non sono impostati i path rimando il messaggio
  readWifiStatus();

  //Function that manages the alarm button
  buttonAlarm();

  ledAlarm();


  readFlameDetectorSensor();

  readMotionDetectorSensor();

  MQTTreadings();

 // Aggiorna l'ora dal server NTP
  timeClient.update(); 
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


void buttonAlarm() {
  if (isButtonPressed() == true) {  // Button pressed
    alarm = !alarm;
    if (alarm == LOW) {
      Serial.println(F("Button: PRESSED, Alarm OFF"));

    } else {
      Serial.println(F("Button: PRESSED, Alarm ON"));
    }
  }
}

void ledAlarm() {
  if (alarm) {                            // If the alarm is active
    digitalWrite(LED_GREEN_ALARM, HIGH);  // Turns on the green LED
    digitalWrite(LED_RED_ALARM, LOW);     // Turns off the red LED
  } else {                                // If the alarm is not active
    digitalWrite(LED_GREEN_ALARM, LOW);   // Turns off the green LED
    digitalWrite(LED_RED_ALARM, HIGH);    // Turns on the red LED
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


void readFlameDetectorSensor() {
  static unsigned long lastFlameStatus = millis();  // init with millis at first call
  static byte lastFlameValue = LOW;                 // Store last flame value

  // Read Flame sensor status every FLAME_STATUS_INTERVAL milliseconds
  if (millis() - lastFlameStatus > FLAME_STATUS_INTERVAL) {
    flameValue = digitalRead(FLAME_DETECTOR);

    if (flameValue == HIGH && alarm == true) {  // if flame is detected
      digitalWrite(FLAME_LED, LOW);             // turn ON FLAME_LED
    } else {
      digitalWrite(FLAME_LED, HIGH);  // turn OFF FLAME_LED
    }

    // Check if there is a transition from 0 to 1 in the flame sensor value
    if (flameValue == HIGH && lastFlameValue == LOW && alarm == true) {

      Serial.println("\nIncendio Rilevato, Salvataggio evento su Database");
      WriteMultiToDB("Allarme! Incendio rilevato");
    }

    lastFlameValue = flameValue;  // Update last flame value
    lastFlameStatus = millis();
  }
}


void readMotionDetectorSensor() {
  static byte lastMotionState = HIGH;                // the previous reading from the input pin
  static unsigned long lastDebounceTime = millis();  // the last time the output pin was toggled
  static unsigned long lastWriteTime = 0;            // last time data was written to the database


  byte currentState = digitalRead(MOTION_DETECTOR);

  if ((millis() - lastDebounceTime) > MOTION_DEBOUNCE_DELAY) {
    if (currentState != motionValue) {
      motionValue = currentState;

      if (motionValue == LOW && alarm == true) {  // if motion is detected
        digitalWrite(MOTION_LED, LOW);            // turn ON MOTION_LED

        // funzione che pubblica tramite mqtt la rilevazione di un movimento
        MQTTpublishMovement();
      } else {
        digitalWrite(MOTION_LED, HIGH);  // turn OFF MOTION_LED
      }

      
      //per fare in modo che una volta che scrive un messaggio aspetta un tempo pari a DB_WRITE_INTERVAL
      if (motionValue == LOW && alarm == true && (millis() - lastWriteTime) > DB_WRITE_INTERVAL) {
        Serial.println("Movimento rilevato, salvataggio evento sul database");
        WriteMultiToDB("Allarme! Movimento rilevato");
        lastWriteTime = millis();  // update last write time
      }
    }
  }

  lastMotionState = currentState;  // Update last motion state
}

// TODO sistemare orario perchè fuso orario sbagliato
// Funzione di supporto per ottenere la data e l'ora formattate
String getFormattedDateTime() {
  static const int timezoneOffset = 7200; // Offset di 1 ora in secondi

  time_t rawTime = timeClient.getEpochTime();
  rawTime += timezoneOffset; // Aggiunge l'offset del fuso orario
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);

  char buffer[20];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          timeInfo->tm_year + 1900,
          timeInfo->tm_mon + 1,
          timeInfo->tm_mday,
          timeInfo->tm_hour,
          timeInfo->tm_min,
          timeInfo->tm_sec);
  return String(buffer);
}
