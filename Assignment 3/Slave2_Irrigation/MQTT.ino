void connectToMQTTBroker() {
  if (!mqttClient.connected()) {
    Serial.print(F("\nConnecting to MQTT broker..."));


    JsonDocument doc;
    doc["mac_address"] = MAC_device;
    doc["board_type"] = "B";  // Change this according to your board type
    doc["message"] = "Unexpected disconnection slave!";

    char buffer[256];
    serializeJson(doc, buffer, 256);

    // setup LWT
    // imposta il messaggio LWT, sul topic LWT viene inviato un messaggio di unexpected disconnettion
    // tutti i client connnessi al broker e iscritti al topic LWT ricevono il messaggio in caso di disconnessionne
    mqttClient.setWill("903604/LWT", buffer, false, 2);  //TODO passare indirizzo dinamico

    while (!mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.print(F("."));
      delay(1000);
    }
    Serial.println(F("\nConnected!"));

    // Send first message on Control Topic
    sendHelloMessage();

    // connected to broker, subscribe topics
    mqttClient.subscribe(MQTT_TOPIC_CONTROL);  // Subscribe to the control topic to receive configurations
    Serial.println(F("Subscribed to control topic!\n"));
  }
}

void sendHelloMessage() {
  MQTT_TOPIC_CONTROL = "903604/general/" + MAC_device;

  DynamicJsonDocument doc(256);
  doc["mac_address"] = MAC_device;
  doc["board_type"] = "B";  // Change this according to your board type

  String message;
  serializeJson(doc, message);

  mqttClient.publish(MQTT_TOPIC_CONTROL, message, false, 2);
  Serial.print("First message on topic: ");
  Serial.print(MQTT_TOPIC_CONTROL + " sent: ");
  Serial.println(message);
}

void mqttMessageReceived(String &topic, String &payload) {
  Serial.print(F("Message received on topic: "));
  Serial.println(topic);
  Serial.print(F("Payload: "));
  Serial.println(payload);

  if (topic.startsWith("903604/general/")) {
    String macAddress = WiFi.macAddress(); //todo si potrebbe mettere MAC_device
    if (topic.endsWith(macAddress)) {
      JsonDocument doc;
      deserializeJson(doc, payload);

      if(doc["status"].as<String>() == "OK"){
        // Extracting paths from the JSON object
        waterLevelTopic = (String)doc["water_level"];
        Serial.print(F("Water level Topic Set To: "));
        Serial.println(waterLevelTopic);

        manualIrrigationTopic = (String)doc["manual_irrigation"];
        Serial.print(F("Manual Irriggation Topic Set To: "));
        Serial.println(manualIrrigationTopic);

        masterComunicationsTopic = (String)doc["master_comunications"];
        Serial.print(F("\nMaster comunications Topic Set To: "));
        Serial.println(masterComunicationsTopic);

        Serial.print(F("\nMaster comunications Topic path saved\n"));
        //una volta ricevuto il messaggio e salvato il topic in cui ricevere le comunicazioni dal master, si sottoscrive al topic delle comunicazioni
        mqttClient.subscribe(masterComunicationsTopic);  // Subscribe to the control topic to receive configurations
        Serial.println(F("Subscribed to control master comunications!\n"));

        String ip = (String)doc["mysql_ip"];
        MYSQL_IP.fromString(ip);
        Serial.print(F("\nMySQL ip Set To: "));
        Serial.println(MYSQL_IP.toString());

        String mysql_user_string = doc["mysql_user"].as<String>();
        mysql_user_string.toCharArray(mysql_user, mysql_user_string.length() + 1);
        Serial.print(F("MySQL User Set To: "));
        Serial.println(mysql_user);

        String mysql_password_string = doc["mysql_password"].as<String>();
        mysql_password_string.toCharArray(mysql_password, mysql_password_string.length() + 1);
        Serial.print(F("MySQL Password Set To: "));
        Serial.println(mysql_password);


        topicPathUpdated = true;
        Serial.print(F("\nSalvataggio percorsi topic SLAVE salvati"));
        Serial.print("\nValore topicPathUpdated: ");
        Serial.print(topicPathUpdated);
        Serial.println("\n\n");

      }else if(doc["status"].as<String>() == "KO"){
        Serial.println("\nComunicazione MQTT con Master fallita --> numero di slave gestiti dal master raggiunto\n");
      }else{
        Serial.println("\nComunicazione MQTT con Master fallita --> errore di comunicazione\n");
      }
    }

  } else if (topic == masterComunicationsTopic) {
    JsonDocument doc;
    deserializeJson(doc, payload);

  //if (doc["manual_irrigation"].as<boolean>())
    // Extracting paths from the JSON object
    manualIrrigationMode = doc["manual_irrigation"].as<boolean>();
    Serial.print(F("Manual Irrigation Mode set to: "));
    Serial.println(manualIrrigationMode);
    refreshLCD(true);

  } else {
    Serial.println(F("MQTT Topic not recognized, message skipped"));
  }
}


void MQTTreadings(bool execute) {
  static unsigned long lastMQTTdataWrite = millis();  // init with millis at first call
  static bool success = false;

  if (millis() - lastMQTTdataWrite > MQTT_WRITE_INTERVAL && topicPathUpdated || execute) {
    Serial.println(F("----------------"));
    Serial.println(F("Invio valori rilevati dai sensori tramite MQTT\n\n"));

    // writing the water level
    JsonDocument doc;
    doc["water_level"] = waterLevel;
    String buffer;
    serializeJson(doc, buffer);
    Serial.print(F("Water level: "));
    Serial.println(buffer);
    // Publish the water level and check for success
    success = mqttClient.publish(waterLevelTopic, buffer, false, 1);
    
    if (success) {
      Serial.println(F("water level published successfully.\n\n"));
    } else {
      Serial.println(F("Error publishing water level.\n\n"));
    }


    // writing the manual irrigation value
    doc.clear();
    doc["manual_irrigation"] = manualIrrigationMode;
    serializeJson(doc, buffer);
    Serial.print(F("Manual Irrigation value: "));
    Serial.println(buffer);
    // Publish themanual irrigation value and check for success
    success = mqttClient.publish(manualIrrigationTopic, buffer, false, 1);
    
    if (success) {
      Serial.println(F("Manual Irrigation published successfully.\n\n"));
    } else {
      Serial.println(F("Error publishing Manual Irrigation.\n\n"));
    }

    lastMQTTdataWrite = millis();
  }
}