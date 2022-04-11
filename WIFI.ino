void WIFI_Pairing(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #ifdef DEBUG
      Serial.println("WIFI : Connection Failed! Rebooting...");
    #endif
    delay(5000);
    ESP.restart();
  }
  digitalWrite(LED_WIFI, HIGH);
}
