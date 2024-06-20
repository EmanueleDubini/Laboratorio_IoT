
void WriteMultiToDB(String message) {

  // connect to MySQL
  if (!conn.connected()) {
    conn.close();
    Serial.println(F("\nConnecting to MySQL..."));
    if (conn.connect(MYSQL_IP, 3306, mysql_user, mysql_password)) {
      Serial.println(F("MySQL connection established."));
    } else {
      Serial.println(F("MySQL connection failed."));
    }
  }

  // log data
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);

  // !motionValue is to invert the motionValue before writing to the database
  sprintf(query, INSERT_DATA, mysql_user, ssid.c_str(), signalStrenght, !motionValue, flameValue, alarm, message.c_str());

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
}