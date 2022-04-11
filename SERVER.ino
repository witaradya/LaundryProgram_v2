void SERVER_Get(String JSONMessage){
    DynamicJsonDocument doc(2048);
    deserializeJson(doc,JSONMessage);
    Serial.println(doc.size());
    
    bool machineSts = doc["machine_status"];
    Serial.println(machineSts);
}


/* 
 * @brief     : GET Data Machine from Server

 * @details   : Get Data Machine from server, specifically Status Machine on or off.
 *              If data status-machine On, it means Controler must activate machine by trigger if.
 *              If data status-machine Off, it means machine is off and nothing to do with the machine.
 *              
 *              If status-machine change from false(0) to true(1) -> setMachineOn = TRUE and machineOn = true
 * 
 * @param     : none
 * 
 * @retval    : none
 *  
 */
void SERVER_getJsonResponse(){
    http.begin(URL_GET); //Specify the URL
    
    int httpCode = http.GET();
    if (httpCode > 0) {
      //Serial.println("GET status mesin"); 
      prevMachineState = machineState;
      DynamicJsonDocument doc(2048);
      deserializeJson(doc,http.getString());

      machineState = doc["machine_status"];
      #ifdef DEBUG
        Serial.print("SERVER : Status Mesin = ");
        Serial.println(machineState);
      #endif
      // If machineState = TRUE and !prevMachine = FALSE and Menit, detik equal to 0
      if(machineState && !prevMachineState && ((menit == 0) && (detik == 0))) {
        setMachineON = true;
        machineOn = true;
        #ifdef DEBUG
          Serial.println("Nyalakan Mesin ...");
        #endif
      }
        // String payload = http.getString();
        // Serial.println(httpCode);
        // Serial.println(payload);
        // SERVER_Get(payload);
    }
  
    else {
      Serial.println("Error on HTTP request");
    }
  
    http.end(); //Free the resources
}

/* 
 * @brief     : Update Machine Data(status on off machine) to Server
 *
 * @details   : Access server using URL_UPDATE address and then send update message that contain machine_status data.
 *              Value of machine_status variable is TRUE(if Washer/Dryer ON) and FALSE(if Washer?Sryer OFF)
 * 
 * @param     : stsMachine (TRUE = machine ON, FALSE = machine OFF)
 * 
 * @retval    : TRUE, if success update data to server
 *              FALSE, if failed update data to server
 */
bool SERVER_Update(bool stsMachine){
  http.begin(URL_UPDATE);

  http.addHeader("accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  
  if(stsMachine) httpResponseCode = http.PUT("{\"machine_status\":\"true\"}");
  else httpResponseCode = http.PUT("{\"machine_status\":\"false\"}"); 
    
  if(httpResponseCode>0){
    String response = http.getString();  
    #ifdef DEBUG 
      Serial.println(httpResponseCode);
      Serial.println(response);          
    #endif
  }
  else{
    #ifdef DEBUG
      Serial.print("Error on sending PUT Request: ");
      Serial.println(httpResponseCode);
    #endif
  }
  
  http.end();

  if(httpResponseCode > 0) return true;
  else return false;
}
