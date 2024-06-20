void handle_root() {
  Serial.print(F("New Client with IP: "));
  Serial.println(server.client().remoteIP().toString());
  server.send(200, F("text/html"), SendHTML(Alarm));
}

void handle_ledon() {
  Alarm = true;  // activate alarm
  displaySystemOn();
  Serial.println(F("Led ON"));
  server.send(200, F("text/html"), SendHTML(Alarm));
}

void handle_ledoff() {
  Alarm = false;  // deactivate alarms
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
                 ".container { display: flex; justify-content: space-around; }\n"
                 ".box { width: 30%; }\n"
                 "</style>\n"
                 "<script>\n"
                 "function submitThresholds(event) {\n"
                 "  event.preventDefault();\n"
                 "  var formData = new URLSearchParams(new FormData(document.getElementById('thresholdForm'))).toString();\n"
                 "  var xhr = new XMLHttpRequest();\n"
                 "  xhr.open('GET', '/setThresholds?' + formData, true);\n"
                 "  xhr.onload = function() {\n"
                 "    if (xhr.status >= 200 && xhr.status < 300) {\n"
                 "      document.getElementById('status').innerText = 'Thresholds updated successfully';\n"
                 "    } else {\n"
                 "      document.getElementById('status').innerText = 'Error updating thresholds';\n"
                 "    }\n"
                 "  };\n"
                 "  xhr.send();\n"
                 "}\n"
                 "</script>\n"
                 "</head>\n"
                 "<body>\n"
                 "<h1>Turning Alarm ON/OFF </h1>\n");

  if (ledstat) {
    ptr += F("<p>Current Alarm Status: ON</p><a class=\"button button-off\" href=\"/OFF\">OFF</a>\n");
  } else {
    ptr += F("<p>Current Alarm Status: OFF</p><a class=\"button button-on\" href=\"/ON\">ON</a>\n");
  }

  // Container to hold sensor, weather, and geolocation information
  ptr += F("<div class=\"container\">\n");

  // Section to show the values read by the sensors
  ptr += F("<div class=\"box\">\n");
  ptr += F("<h3>Sensor Values</h3>");
  ptr += F("<p>Humidity: ") + String(h) + F("%</p>\n");
  ptr += F("<p>Temperature: ") + String(t) + F(" C</p>\n");
  ptr += F("<p>Perceived Temperature: ") + String(hic) + F(" C</p>\n");
  ptr += F("<p>WiFi Signal Strength: ") + String(signalStrenght) + F(" dBm</p>\n");
  ptr += F("<p>Light Sensor Value: ") + String(lightSensorValue) + F("</p>\n");
  ptr += F("</div>\n");

  // Section to show the geolocation information
  ptr += F("<div class=\"box\">\n");
  ptr += F("<h3>IP Geolocalizated Position</h3>");
  ptr += F("<p>City: ") + currentCity + F("</p>\n");
  ptr += F("<p>Country Code: ") + currentCountryCode + F("</p>\n");
  ptr += F("<p>Country: ") + currentCountry + F("</p>\n");
  ptr += F("<p>Region: ") + currentRegionName + F("</p>\n");
  ptr += F("<p>ZIP: ") + currentZip + F("</p>\n");
  ptr += F("<p>Timezone: ") + currentTimezone + F("</p>\n");
  ptr += F("</div>\n");

  // Add weather information
  ptr += F("<div class=\"box\">\n");
  ptr += F("<h3>Current Online Weather</h3>");
  ptr += F("<p>Location: ") + locationName + F("</p>\n");
  ptr += F("<p>Temperature: ") + String(tempAPI) + F(" C</p>\n");
  ptr += F("<p>Humidity: ") + String(humidityAPI) + F(" %</p>\n");
  ptr += F("<p>Weather: ") + weatherAPI + F("</p>\n");
  ptr += F("<p>Weather description: ") + weatherDescriptionAPI + F("</p>\n");
  ptr += F("<p>Sunrise (UNIX timestamp): ") + String(sunrise) + F("</p>\n");
  ptr += F("<p>Sunset (UNIX timestamp): ") + String(sunset) + F("</p>\n");
  ptr += F("<p>Temperature min. (C): ") + String(tempMinAPI) + F("</p>\n");
  ptr += F("<p>Temperature max. (C): ") + String(tempMaxAPI) + F("</p>\n");
  ptr += F("<p>Wind speed (m/s): ") + String(windSpeed) + F("</p>\n");
  ptr += F("</div>\n");

  ptr += F("</div>\n");  // Close container

  // Add form for setting thresholds
  ptr += F("<br><h3>Set Thresholds</h3>"
           "<form id='thresholdForm' onsubmit='submitThresholds(event); return false;'>\n"
           "Temperature: <input type=\"number\" name=\"temp\" value=\"")
         + String(SogliaTemperatura) + F("\"><br>\n")
         + F("<p>Temperature threshold Range: -50 to 150</p>\n")  // Added subtitle for temperature range
         + F("Humidity: <input type=\"number\" name=\"hum\" value=\"")
         + String(SogliaUmidita) + F("\"><br>\n")
         + F("<p>Humidity threshold Range: 0 to 100</p>\n")  // Added subtitle for humidity range, if necessary
         + F("Light: <input type=\"number\" name=\"light\" value=\"")
         + String(SogliaLuce) + F("\"><br>\n")
         + F("<p>Light threshold Range: 0 to 1024</p>\n")  // Added subtitle for light range, if necessary
         + F("<input type=\"submit\" value=\"Set Thresholds\">\n")
         + F("</form>\n<p id='status'></p>\n");


  
  for (int i = 0; i < 3; i++) {
    if (MAC_slave_Type[i] == "A") {
      if (LWT_slave[i]) {
        ptr += F("<br><h3>System Status</h3>");
        ptr += F("<p>Sistema di allarme Offline</p>\n");
      } else {
        Alarm_HTML(ptr, i);
      }
    }
  }

  for (int i = 0; i < 3; i++) {
    if (MAC_slave_Type[i] == "B") {
      if (LWT_slave[i]) {
        ptr += F("<br><h3>System Status</h3>");
        ptr += F("<p>Sistema di irrigazione Offline</p>\n");
      } else {
        Irrigation(ptr, i);
      }
    }
  }

  ptr += F("</body>\n</html>\n");

  return ptr;
}

void Irrigation(String &ptr, int i) {
  // Add flame, movement, and alarm_storage values
  ptr += F("<br><h3>Irrigation system</h3>");

  ptr += F("<p>Percentuale di riempimento della cisterna: ") + String(waterLevel[i]) + F("%</p>\n");

  // Aggiungere un pulsante per attivare/disattivare l'irrigazione manuale
  ptr += F("<form action=\"/toggle_irrigation\" method=\"POST\">");
  ptr += F("<input type=\"hidden\" name=\"index\" value=\"") + String(i) + F("\">");
  if (manualIrrigation[i]) {
    ptr += F("<input type=\"submit\" value=\"Attiva Irrigazione Manuale\">");
  } else {
    ptr += F("<input type=\"submit\" value=\"Disattiva Irrigazione Manuale\">");
  }
  ptr += F("</form>");
}

void Alarm_HTML(String &ptr, int i) {
  // Add flame, movement, and alarm_storage values
  ptr += F("<br><h3>Storage Sensors</h3>");

  ptr += F("<p>Alarm Storage: ") + String(alarm_storage[i] ? "Enabled" : "Disabled") + F("</p>\n");

  // Add button to toggle alarm_storage
  if (alarm_storage[i]) {
    ptr += F("<a class=\"button button-off\" href=\"/AlarmStorageOFF\">Disable</a>");
    ptr += F("<p>NOTA: La disattivazione del sistema di allarme richiede una riattivazione manuale dal dispositivo</p>\n\n");
  } else {
    ptr += F("<p>Il sistema di allarme e' disattivato, necessaria riattivazione manuale</p>\n");
  }

  ptr += F("<p>Flame Detection: ") + String(flame[i] == 0 ? "No Flames Detected" : "Flames Detected") + F("</p>\n");
  ptr += F("<p>Movement Detection: Last movement detected at ") + movement_timestamp[i] + F("</p>\n");

  ptr += F("<form action=\"/submitEmail\" method=\"POST\">");
  ptr += F("<label for=\"userEmail\">Inserire una email:</label><br>");
  ptr += F("<input type=\"text\" id=\"userEmail\" name=\"userEmail\"><br><br>");
  ptr += F("<input type=\"submit\" value=\"Submit\">");
  ptr += F("</form>");

  ptr += F("<p>Email inserita: ") + userEmail + F("</p>\n");
}

void handleSubmitEmail() {
  if (server.method() == HTTP_POST) {
    if (server.hasArg("userEmail")) {
      userEmail = server.arg("userEmail");
      Serial.println("Received string: " + userEmail);
      for (int i = 0; i < 3; i++) {  //publish email only for alarm boards
        if (MAC_slave_Type[i] == "A") {
          publishPath(MAC_slave[i], "A");
        }
      }
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);  // Redirect al root
}

void handle_AlarmStorageON() {
  for (int i = 0; i < 3; i++) {  //publish email only for alarm boards
    if (MAC_slave_Type[i] == "A" && MAC_slave[i] != "No Mac Slave connected") {
      alarm_storage[i] = true;
      server.send(200, F("text/html"), SendHTML(Alarm));
      Serial.println(F("Alarm Storage Enabled"));

      // Publish MQTT update for the alarm status
      StaticJsonDocument<MQTT_BUFFER_SIZE> doc;
      doc["alarm_enabled"] = alarm_storage[i];
      String buffer;
      serializeJson(doc, buffer);
      Serial.print(F("JSON message: "));
      Serial.println(buffer);
      mqttClient.publish("903604/" + MAC_slave[i] + "/master_comunication", buffer, false, 2);
    }
  }
}

void handle_AlarmStorageOFF() {
  for (int i = 0; i < 3; i++) {  //publish email only for alarm boards
    if (MAC_slave_Type[i] == "A" && MAC_slave[i] != "No Mac Slave connected") {
      alarm_storage[i] = false;
      server.send(200, F("text/html"), SendHTML(Alarm));
      Serial.println(F("Alarm Storage Disabled"));

      // Publish MQTT update for the alarm status
      StaticJsonDocument<MQTT_BUFFER_SIZE> doc;
      doc["alarm_enabled"] = alarm_storage[i];
      String buffer;
      serializeJson(doc, buffer);
      Serial.print(F("JSON message: "));
      Serial.println(buffer);
      mqttClient.publish("903604/" + MAC_slave[i] + "/master_comunication", buffer, false, 2);
    }
  }
}

void handleToggleIrrigation() {
  if (server.method() == HTTP_POST) {
    // Estrai il valore dell'indice dalla richiesta HTTP
    int index = server.arg("index").toInt();
    manualIrrigation[index] = !manualIrrigation[index];

    // Publish MQTT update for the irrigation status
    StaticJsonDocument<MQTT_BUFFER_SIZE> doc;
    doc["manual_irrigation"] = !manualIrrigation[index];
    String buffer;
    serializeJson(doc, buffer);
    Serial.print(F("JSON message: "));
    Serial.println(buffer);
    mqttClient.publish("903604/" + MAC_slave[index] + "/master_comunication", buffer, false, 2);

    // Reindirizza alla pagina principale
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}
