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
void SERVER_getJsonResponse(String URLget, String param){
    http.begin(URLget); //Specify the URL
    
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      DynamicJsonDocument doc(2048);
      deserializeJson(doc,http.getString());
      
      if(param == "machine_status"){
        prevMachineState = machineState;
        machineState = doc["machine_status"];
        #ifdef DEBUG
          Serial.print("SERVER : Status Mesin = ");
          Serial.print(machineState);
          Serial.print("\t");
          Serial.println(httpCode);
        #endif
        // If machineState = TRUE and !prevMachine = FALSE and Menit, detik equal to 0
        if(machineState && !prevMachineState && ((menit == 0) && (detik == 0))) {
          #ifdef DEBUG
            Serial.println("DETEKSI 1");
          #endif
          packet = doc["is_packet"];
          TON_MACHINE = doc["price_time"];
          prevMachineState = 0;
          machineState = 0;
          
          if(TON_MACHINE){
            #ifdef DEBUG
              Serial.println("BENAR 1");
            #endif
            
            machineState = 1;
            
//            digitalWrite(2, HIGH);
            
            if(packet) PACKET = "true";
            else PACKET = "false";
            URL_Server = (String)URL + (String)GET_ID + (String)PACKET + (String)GET_ID_2 + (String)MACHINE_ID + (String)GET_ID_3 + (String)STORE;
            Serial.println(URL_Server);
            SERVER_getJsonResponse(URL_Server, "_id");
  
            IsTransaction = true;          
            setMachineON = true;
            machineOn = true;
            
            EEPROM.write(STS_ADDR, machineState);
            EEPROM.write(MINUTE_ADDR, TON_MACHINE);
            EEPROM.write(MICOM_STAGE, 1);
            EEPROM.write(PACKET_ADDR, packet);
            EEPROM.commit();
            
            #ifdef DEBUG
              Serial.print("Nyalakan Mesin ...  ");
              Serial.print("Paket : ");
              Serial.print(packet);
              Serial.print("\tTON : ");
              Serial.println(TON_MACHINE);
            #endif
  
            // Update stage machine to 2 = Machine working
            URL_Server = (String) URL + (String) GET_MACHINE + (String) MACHINE_ID;
            DB_Message = "{\"debug_machine\":2}";
            SERVER_Update(URL_Server, DB_Message);
          }
        }
      }
      else if(param == "_id"){
        int index = doc.size();
        #ifdef DEBUG
          Serial.print("Size JSON : ");
          Serial.println(index);
        #endif
        String id = doc[index - 1][param];
        
        Transaction_ID = id;
        
        #ifdef DEBUG
          Serial.print("Transaction_ID : ");
          Serial.println(Transaction_ID);
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
bool SERVER_Update(String UrlUpdate, String message){
  http.begin(UrlUpdate);

  http.addHeader("accept", "application/json");
  http.addHeader("Content-Type", "application/json");

  httpResponseCode = http.PATCH(message);

  if(httpResponseCode > 0){
    #ifdef DEBUG
//      String response = http.getString();   
//      Serial.print(httpResponseCode);
//      Serial.println(response);          
    #endif
    http.end();
    if(httpResponseCode == 404)return false;
    else return true;
  }
  else{
    #ifdef DEBUG
      Serial.print("Error on sending PUT Request: ");
      Serial.println(httpResponseCode);
    #endif
    http.end();
    return false;
  }
}
