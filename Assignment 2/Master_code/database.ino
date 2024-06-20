int WriteMultiToDB(char ssid[], int signalStrenght, int tilt, float h, float t, int lightSensorValue, int Alarm) {
  //alarm indicates if the monitoring system is active, warning indicates if a danger is present (such as a value detected above a certain threshold)
  static int warning = 0;
  static unsigned long lastDbWrite = millis();  // init with millis at first call, number of milliseconds since the processor/board was powered up

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

    //set the value of the alarm. every commbination of alarm sensors has a corresponding number.
    if ((h > SogliaUmidita && t > SogliaTemperatura && lightSensorValue < SogliaLuce) && Alarm == true) {
      warning = 7;  //Value if every threshold is over
    } else if ((h > SogliaUmidita && lightSensorValue < SogliaLuce) && Alarm == true) {
      warning = 6;  //Value if humidity and light thresholds are over
    } else if ((t > SogliaTemperatura && lightSensorValue < SogliaLuce) && Alarm == true) {
      warning = 5;  //Value if temperature and light thresholds are over
    } else if ((h > SogliaUmidita && t > SogliaTemperatura) && Alarm == true) {
      warning = 4;  //Value if humidity, temperature thresholds are over
    } else if ((lightSensorValue < SogliaLuce) && Alarm == true) {
      warning = 3;  //Value if  light thresholds are over
    } else if (h > SogliaUmidita && Alarm == true) {
      warning = 2;  //Value if humidity thresholds are over
    } else if (t > SogliaTemperatura && Alarm == true) {
      warning = 1;  //Value if temperature thresholds are over
    } else {
      // Se nessuna delle condizioni precedenti è vera
      warning = 0;
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
  return 0;  //if it didn't run the timer code
}