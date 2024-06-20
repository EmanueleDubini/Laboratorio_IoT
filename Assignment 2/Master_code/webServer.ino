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

// Aggiorna la funzione SendHTML per includere i nuovi valori
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

  // Section to show the values read by the sensors
  ptr += F("<h3>Sensor Values</h3>");
  ptr += F("<p>Humidity: ") + String(h) + F("%</p>\n");
  ptr += F("<p>Temperature: ") + String(t) + F(" C</p>\n");
  ptr += F("<p>Perceived Temperature: ") + String(hic) + F(" C</p>\n");
  ptr += F("<p>WiFi Signal Strength: ") + String(signalStrenght) + F(" dBm</p>\n");
  ptr += F("<p>Light Sensor Value: ") + String(lightSensorValue) + F("</p>\n");


  // Add weather information
  ptr += F("<h3>Current Online Weather</h3>");
  ptr += F("<p>Location: ") + locationName + F("</p>\n");
  ptr += F("<p>Country: ") + countryName + F("</p>\n");
  ptr += F("<p>Temperature: ") + String(tempAPI) + F(" C</p>\n");
  ptr += F("<p>Humidity: ") + String(humidityAPI) + F(" %</p>\n");
  ptr += F("<p>Weather: ") + weatherAPI + F("</p>\n");
  ptr += F("<p>Weather description: ") + weatherDescriptionAPI + F("</p>\n");
  ptr += F("<p>Sunrise (UNIX timestamp): ") + String(sunrise) + F("</p>\n");
  ptr += F("<p>Sunset (UNIX timestamp): ") + String(sunset) + F("</p>\n");
  ptr += F("<p>Temperature min. (C): ") + String(tempMinAPI) + F("</p>\n");
  ptr += F("<p>Temperature max. (C): ") + String(tempMaxAPI) + F("</p>\n");
  ptr += F("<p>Wind speed (m/s): ") + String(windSpeed) + F("</p>\n");


  // Add form for setting thresholds
  ptr += F("<br><h3>Set Thresholds</h3>"
           "<form id='thresholdForm' onsubmit='submitThresholds(event); return false;'>\n"
           "Temperature: <input type=\"number\" name=\"temp\" value=\"")
         + String(SogliaTemperatura) + F("\"><br>\n")
         + F("<p>Temperature treshold Range: -50 to 150</p>\n")  // Aggiunto sottotitolo per il range di temperatura
         + F("Humidity: <input type=\"number\" name=\"hum\" value=\"")
         + String(SogliaUmidita) + F("\"><br>\n")
         + F("<p>Humidity treshold Range: 0 to 100</p>\n")  // Aggiunto sottotitolo per il range di umidità, se necessario
         + F("Light: <input type=\"number\" name=\"light\" value=\"")
         + String(SogliaLuce) + F("\"><br>\n")
         + F("<p>Light treshold Range: 0 to 1024</p>\n")  // Aggiunto sottotitolo per il range di luce, se necessario
         + F("<input type=\"submit\" value=\"Set Thresholds\">\n")
         + F("</form>\n<p id='status'></p>\n");

  // Check if LWT_slave1 is true
  if (LWT_slave[0]) {
    ptr += F("<br><h3>System Status</h3>");
    ptr += F("<p>Sistema di allarme Offline</p>\n");
  } else {

    // Add flame, movement, and alarm_storage values
    ptr += F("<br><h3>Storage Sensors</h3>");

    ptr += F("<p>Alarm Storage: ") + String(alarm_storage[0] ? "Enabled" : "Disabled") + F("</p>\n");

    // Add button to toggle alarm_storage
    if (alarm_storage[0]) {
      ptr += F("<a class=\"button button-off\" href=\"/AlarmStorageOFF\">Disable</a>\n");
    } else {
      ptr += F("<a class=\"button button-on\" href=\"/AlarmStorageON\">Enable</a>\n");
    }

    ptr += F("<p>Flame Detection: ") + String(flame[0] == 0 ? "No Flames Detected" : "Flames Detected") + F("</p>\n");
    ptr += F("<p>Movement Detection: Last movement detected at ") + movement_timestamp[0] + F("</p>\n");
  }
  ptr += F("</body>\n</html>\n");

  return ptr;
}

void handle_AlarmStorageON() {
  alarm_storage[0] = true;
  server.send(200, F("text/html"), SendHTML(Alarm));
  Serial.println(F("Alarm Storage Enabled"));

  // Publish MQTT update for the alarm status
  StaticJsonDocument<MQTT_BUFFER_SIZE> doc;
  doc["alarm_enabled"] = alarm_storage[0];
  String buffer;
  serializeJson(doc, buffer);
  Serial.print(F("JSON message: "));
  Serial.println(buffer);
  mqttClient.publish("903604/" + MAC_slave[0] + "/master_comunication", buffer, false, 2);
}

void handle_AlarmStorageOFF() {
  alarm_storage[0] = false;
  server.send(200, F("text/html"), SendHTML(Alarm));
  Serial.println(F("Alarm Storage Disabled"));

  // Publish MQTT update for the alarm status
  StaticJsonDocument<MQTT_BUFFER_SIZE> doc;
  doc["alarm_enabled"] = alarm_storage[0];
  String buffer;
  serializeJson(doc, buffer);
  Serial.print(F("JSON message: "));
  Serial.println(buffer);
  mqttClient.publish("903604/" + MAC_slave[0] + "/master_comunication", buffer, false, 2);
}