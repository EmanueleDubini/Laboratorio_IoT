void connectToMQTTBroker() {
  if (!mqttClient.connected()) {  // not connected
    Serial.print(F("\nConnecting to MQTT broker..."));
    while (!mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.print(F("."));
      delay(1000);
    }
    Serial.println(F("\nConnected!"));

    mqttClient.subscribe(MQTT_CONTROL);
    Serial.println(F("\nSubscribed to control topic!"));

    // Subscribe to LWT topic
    mqttClient.subscribe(MQTT_TOPIC_LWT);
    Serial.println(F("\nSubscribed to LWT topic!"));
  }
}

void mqttMessageReceived(String &topic, String &payload) {
  // this function handles a message from the MQTT broker
  Serial.println("Incoming MQTT message: " + topic + " - " + payload);
  JsonDocument doc1;
  deserializeJson(doc1, payload);

  if (topic.startsWith("903604/general/")) {
    if (doc1["mac_address"] != MAC_master && doc1["board_type"] == "A") {
      // controlla se il mac ricevuto nel messaggio è gia presente in uno slot dell'array MAC_slave[] oppure se il primo slot è vuoto
      if (MAC_slave[0] == "No Mac Slave connected" || MAC_slave[0] == String(doc1["mac_address"])) {
        MAC_slave[0] = String(doc1["mac_address"]);
        MAC_slave_Type[0] = "A";
        Serial.println("aggiunto indirizzo mac: " + String(doc1["mac_address"]) + "al MAC_slave[0]");
        publishPath(MAC_slave[0], "A");
      }

      else if (MAC_slave[1] == "No Mac Slave connected" || MAC_slave[1] == String(doc1["mac_address"])) {
        MAC_slave[1] = String(doc1["mac_address"]);
        MAC_slave_Type[1] = "A";

        Serial.println("aggiunto indirizzo mac: " + String(doc1["mac_address"]) + "al MAC_slave[1]");
        publishPath(MAC_slave[1], "A");
      }

      else if (MAC_slave[2] == "No Mac Slave connected" || MAC_slave[2] == String(doc1["mac_address"])) {
        MAC_slave[2] = String(doc1["mac_address"]);
        MAC_slave_Type[2] = "A";

        Serial.println("aggiunto indirizzo mac: " + String(doc1["mac_address"]) + "al MAC_slave[2]");
        publishPath(MAC_slave[2], "A");

      } else {  //gestione in caso si provi a collegare più slave di quanto supportato
        String MAC_error = String(doc1["mac_address"]);
        Serial.print("Max number of slave board reached ");
        StaticJsonDocument<MQTT_BUFFER_SIZE> doc;
        doc["status"] = "KO";

        String buffer;
        serializeJson(doc, buffer);
        Serial.print(F("JSON message: "));
        Serial.println(buffer);
        mqttClient.publish(MQTT_TOPIC_CONTROL + MAC_error, buffer, false, 2);  //nella versione statica aggiungere retained = true
      }
    } if (doc1["mac_address"] != MAC_master && doc1["board_type"] == "B") {
      // caso in cui si collega una scheda di tipo B
      // controlla se il mac ricevuto nel messaggio è gia presente in uno slot dell'array MAC_slave[] oppure se il primo slot è vuoto
      if (MAC_slave[0] == "No Mac Slave connected" || MAC_slave[0] == String(doc1["mac_address"])) {
        MAC_slave[0] = String(doc1["mac_address"]);
        MAC_slave_Type[0] = "B";
        Serial.println("aggiunto indirizzo mac: " + String(doc1["mac_address"]) + "al MAC_slave[0]");
        publishPath(MAC_slave[0], "B");
      }

      else if (MAC_slave[1] == "No Mac Slave connected" || MAC_slave[1] == String(doc1["mac_address"])) {
        MAC_slave[1] = String(doc1["mac_address"]);
        MAC_slave_Type[1] = "B";

        Serial.println("aggiunto indirizzo mac: " + String(doc1["mac_address"]) + "al MAC_slave[1]");

        publishPath(MAC_slave[1], "B");
      }

      else if (MAC_slave[2] == "No Mac Slave connected" || MAC_slave[2] == String(doc1["mac_address"])) {
        MAC_slave[2] = String(doc1["mac_address"]);
        MAC_slave_Type[2] = "B";

        Serial.println("aggiunto indirizzo mac: " + String(doc1["mac_address"]) + "al MAC_slave[2]");
        publishPath(MAC_slave[2], "B");

      } else {  //gestione in caso si provi a collegare più slave di quanto supportato
        String MAC_error = String(doc1["mac_address"]);
        Serial.print("Max number of slave board reached ");
        StaticJsonDocument<MQTT_BUFFER_SIZE> doc;
        doc["status"] = "KO";

        String buffer;
        serializeJson(doc, buffer);
        Serial.print(F("JSON message: "));
        Serial.println(buffer);
        mqttClient.publish(MQTT_TOPIC_CONTROL + MAC_error, buffer, false, 2);  //nella versione statica aggiungere retained = true
      }
    }
  }

  for (int i = 0; i < 3; i++) {
    if (topic.startsWith("903604/" + MAC_slave[i])) {
      if (topic.endsWith("/flame")) {
        JsonDocument doc;
        deserializeJson(doc, payload);

        flame[i] = doc["flame_detection"];

        Serial.print(F("Flame Detection Status: "));
        Serial.println(flame[i]);

        if (LWT_slave[i] == true) {
          LWT_slave[i] = false;
          displaySlaveOn();
        }
      } else if (topic.endsWith("/movement")) {
        JsonDocument doc;
        deserializeJson(doc, payload);

        movement[i] = doc["movement_detection"];
        movement_timestamp[i] = String(doc["movement_timestamp"]);
        Serial.print(F("Movement Detection Status: "));
        Serial.println(movement[i]);

        if (LWT_slave[i] == true) {
          LWT_slave[i] = false;
          displaySlaveOn();
        }
      } else if (topic.endsWith("/alarm_enabled")) {
        JsonDocument doc;
        deserializeJson(doc, payload);

        alarm_storage[i] = doc["alarm_enabled"];

        Serial.print(F("Alarm Status: "));
        Serial.println(alarm_storage[i]);

        if (LWT_slave[i] == true) {
          LWT_slave[i] = false;
          displaySlaveOn();
        }
      } else if (topic.endsWith("/water_level")) {
        JsonDocument doc;
        deserializeJson(doc, payload);

        waterLevel[i] = doc["water_level"];

        Serial.print(F("Water Level Status: "));
        Serial.println(waterLevel[i]);

        if (LWT_slave[i] == true) {
          LWT_slave[i] = false;
          displaySlaveOn();
        } else if (topic.endsWith("/manual_irrigation")) {
        JsonDocument doc;
        deserializeJson(doc, payload);

        manualIrrigation[i] = doc["manual_irrigation"];

        Serial.print(F("Manual irrigation: "));
        Serial.println(waterLevel[i]);

        if (LWT_slave[i] == true) {
          LWT_slave[i] = false;
          displaySlaveOn();
        }
      }
    }
  }
  }

  // Handling LWT messages
  if (topic == MQTT_TOPIC_LWT) {
  
    Serial.println("RICEVUTO UN LWT:");

    JsonDocument doc1;
    deserializeJson(doc1, payload);

    Serial.println("dispositivo da scollegare: " + String(doc1["mac_address"]));

    if (MAC_slave[0] == String(doc1["mac_address"])) {
      Serial.println("Slave disconnection detected: " + payload);
      LWT_slave[0] = true;
      MAC_slave[0] = "No Mac Slave connected";
      MAC_slave_Type[0] = "-";
      displaySlaveOff();
    } else if (MAC_slave[1] == String(doc1["mac_address"])) {
      Serial.println("Slave disconnection detected: " + payload);
      LWT_slave[1] = true;
      MAC_slave[1] = "No Mac Slave connected";
      MAC_slave_Type[1] = "-";
      displaySlaveOff();
    } else if (MAC_slave[2] == String(doc1["mac_address"])) {
      Serial.println("Slave disconnection detected: " + payload);
      LWT_slave[2] = true;
      MAC_slave[2] = "No Mac Slave connected";
      MAC_slave_Type[2] = "-";
      displaySlaveOff();
    }

    else {
      Serial.println("MQTT Topic not recognised, message skipped");
    }
  }
}

void publishPath(String MAC_path, String boardType) {
  StaticJsonDocument<MQTT_BUFFER_SIZE> doc;
  String buffer;

  if (boardType == "A") {
    doc["mac_address"] = WiFi.macAddress();
    doc["flame_detection"] = "903604/" + String(MAC_path) + "/flame";
    doc["movement_detection"] = "903604/" + String(MAC_path) + "/movement";
    doc["alarm_enabled"] = "903604/" + String(MAC_path) + "/alarm_enabled";
    doc["master_comunications"] = "903604/" + String(MAC_path) + "/master_comunication";
    doc["mysql_ip"] = IPAddress(MYSQL_IP).toString();
    doc["mysql_user"] = String(MYSQL_USER);
    doc["mysql_password"] = String(MYSQL_PASS);
    doc["email"] = String(userEmail);
    doc["status"] = "OK";

    serializeJson(doc, buffer);
    Serial.print(F("JSON message: "));
    Serial.println(buffer);
    mqttClient.publish(MQTT_TOPIC_CONTROL + MAC_path, buffer, false, 2);  //nella versione statica aggiungere retained = true

    mqttClient.subscribe("903604/" + MAC_path + "/#");

  } else {
    //Board_type == "B"
    doc["mac_address"] = WiFi.macAddress();
    doc["water_level"] = "903604/" + String(MAC_path) + "/water_level";
    doc["manual_irrigation"] = "903604/" + String(MAC_path) + "/manual_irrigation";
    doc["master_comunications"] = "903604/" + String(MAC_path) + "/master_comunication";
    doc["mysql_ip"] = IPAddress(MYSQL_IP).toString();
    doc["mysql_user"] = String(MYSQL_USER);
    doc["mysql_password"] = String(MYSQL_PASS);
    doc["status"] = "OK";

    serializeJson(doc, buffer);
    Serial.print(F("JSON message: "));
    Serial.println(buffer);
    mqttClient.publish(MQTT_TOPIC_CONTROL + MAC_path, buffer, false, 2);  //nella versione statica aggiungere retained = true

    mqttClient.subscribe("903604/" + MAC_path + "/#");
  }
}
