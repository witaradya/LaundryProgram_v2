void OTA_Init(){
  #ifdef WASHER_1
    ArduinoOTA.setHostname("KL_WASHER_1_TITAN");
  #endif

  #ifdef WASHER_3
    ArduinoOTA.setHostname("KL_WASHER_3");
  #endif

  #ifdef WASHER_5
    ArduinoOTA.setHostname("KL_WASHER_5");
  #endif

  #ifdef DRYER_2
    ArduinoOTA.setHostname("KL_DRYER_2_TITAN");
  #endif

  #ifdef DRYER_4
    ArduinoOTA.setHostname("KL_DRYER_4");
  #endif

  #ifdef DRYER_6
    ArduinoOTA.setHostname("KL_DRYER_6");
  #endif

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();  

  #ifdef DEBUG
    Serial.println("WIFI, OTA : Ready");
    Serial.print("WIFI, OTA : IP ");
    Serial.println(WiFi.localIP());
  #endif
}
