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
  lcd.print(h);
  lcd.print("% ");

  lcd.print("WL:");
  lcd.print(waterLevel);
  lcd.print("%");
}

void refreshLCD(bool istantExecute) {
  if (millis() - lastLcdRefresh > LCD_REFRESH_INTERVAL || istantExecute) {  // Refresh LCD every interval or instant execution

    Serial.print("\nAGGIORNO DISPLAY\n");
    lcd.clear();

    if (!autoIrrigationMode && !manualIrrigationMode) {
      lcd.setCursor(0, 0);
      lcd.print("H:");
      lcd.print(h);
      lcd.print("% ");

      lcd.print("WL:");
      lcd.print(waterLevel);
      lcd.print("%");

      lcd.setCursor(0, 1);
      if (!noWater) {
        lcd.print("irr. manuale -->");
      } else {
        lcd.print("acqua esaurita");
      }
    } else if (manualIrrigationMode) {
      lcd.setCursor(0, 0);
      lcd.print("Irr. manuale ON");
    } else if (autoIrrigationMode) {
      lcd.setCursor(0, 0);
      lcd.print("Irr. automatica");
      lcd.setCursor(0, 1);
      lcd.print("in corso... H:");
      lcd.print(h);
      lcd.print("%");
    }

    lastLcdRefresh = millis();  // Update last execution millis
  }
}

void showNoWaterMessageLCD() {
  // Clear display and write the message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Acqua insuffic.");
  lcd.setCursor(0, 1);
  lcd.print("Irrigazione OFF");

  lastLcdRefresh = millis();
}