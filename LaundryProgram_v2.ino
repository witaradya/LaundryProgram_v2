/*
 * 11 MEI 2022
 * Witaradya Adhi Dharma
 */

#define DEBUG

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EEPROM.h"

/*
 * Choose one to activate 1 device that will you use
 */
#define MACHINE_ID          "1" // WASHER TITAN no.1
//#define MACHINE_ID          "2" // DRYER TITAN no.2
//#define MACHINE_ID          "3" // WASHER no.3
//#define MACHINE_ID          "4" // DRYER no.4
//#define MACHINE_ID          "5" // WASHER no.5
//#define MACHINE_ID          "6" // DRYER no.6

#define STORE "1"   //Klaseman Laundry
//#define STORE "2"   //Solo Laundry

#define URL                 "https://api.kontenbase.com/query/api/v1/b7fadcc8-3706-465a-90cb-39880d75338f/"

#define GET_MACHINE         "Machine/"
#define GET_ID              "Transaction?transaction_finish=false&is_packet="
#define GET_ID_2            "&transaction_id_machine="
#define GET_ID_3            "&transaction_store="
#define UPDATE_TRANSACTION  "Transaction/"

#define LED_WIFI      19
#define RESET_BTN     34
#define PIN_MACHINE   25

#define STS_ADDR    0
#define MINUTE_ADDR 1

HTTPClient http;

TaskHandle_t Task1;

const char* ssid = "Timtom";
const char* password = "tingtong9#";

// TIMER
unsigned long prevTime, currentTime;
byte detik, menit, machineSts;
uint8_t TON_MACHINE = 0;

//SERVER MESIN
String URL_Server;
String DB_Message;
int httpResponseCode;
bool machineState = false, prevMachineState = false;
bool setMachineON = false, machineOn = false;
bool updateServer = false;

//SERVER TRANSAKSI
String Transaction_ID;
String PACKET;
bool packet = false;
bool updateEEPROM = false;
bool killTransaction = false;

//RESET BUTTON
bool paksaNyala = false;
uint8_t lastState = HIGH;
uint8_t currentState;
unsigned long pressTime, rilisTime;

//EEPROM
/*
   @brief     : EEPROM Initialization Function

   @details   : This function used to start EEPROM function, read the latest data that saved in EEPROM

   @param     : none

   @retval    : none
*/
void EEPROM_Init() {
  if (!EEPROM.begin(2)) {
    Serial.println("Failed to init EEPROM");
  }

  machineSts = EEPROM.read(STS_ADDR);
  TON_MACHINE = EEPROM.read(MINUTE_ADDR);

  Serial.print("EEPROM_Init : "); Serial.print(machineSts);
  Serial.print("\t"); Serial.println(menit);

  // If machineSts = 1, menit != 0, and detik != 0
  // It means, in the past, machine was on but suddenly power is off, so I will continue to turn on the machine.
  // But the machine is not tirggered anymore. So, setMachine is TRUE to continue the timer and
  // machineOn is FALSE to disable trigger function
  if (machineSts == 1 && (menit > 0)) {
    setMachineON = true;
    machineOn = false;
  }
}

/*
   @brief   : Machine Trigger function

   @details : This function used to trigger Washer/ Dryer by turn on PIN_MACHINE(GPIO25) for 50 millisecond and then turn off.
              To trigger the Washer/Dryer, machineOn variable must be worth TRUE. machineOn will be TRUE if

   @param   : none

   @retval  : none
*/
void MACHINE_on() {
  if (machineOn) {
    #ifdef DEBUG
      Serial.println("MACHINE_on : Mesin Dinyalakan");
    #endif
    for(uint8_t aa = 0; aa < 3; aa++){
      digitalWrite(PIN_MACHINE, HIGH);
      delay(50);
      digitalWrite(PIN_MACHINE, LOW);
      delay(2000);
    }

    machineOn = false;
  }
}

/*
   @brief   : Button By Pass Function
   @details : Used to handle when button byPass is pressed in 5 second or more
   @param   : none
   @retval  : none
*/
void Button_ByPass() {
  currentState = digitalRead(RESET_BTN);
  if (lastState == HIGH && currentState == LOW)pressTime = millis();
  else if (lastState == LOW && currentState == HIGH) {
    rilisTime = millis();
    if ((rilisTime - pressTime) > 5000) {
      paksaNyala = true;
      
      #ifdef DEBUG
        Serial.println("KillTransaction : DIPENCET !!!");
      #endif
    }
  }
  lastState = currentState;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_MACHINE, OUTPUT);
  digitalWrite(PIN_MACHINE, LOW);
  delay(100);

  pinMode(2, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);

  pinMode(RESET_BTN, INPUT);

  WIFI_Pairing();

  EEPROM_Init();

  xTaskCreatePinnedToCore(
    Task1code,   /* Task function. */
    "Task1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(500);

  OTA_Init();

  // Init TON_MACHINE based machine type
  if((MACHINE_ID == "1") || (MACHINE_ID == "3") || (MACHINE_ID == "5"))TON_MACHINE = 2;
  else TON_MACHINE = 2;
}

// Devide 2 task in 2 core
// Task1(void loop) have a task to control WiFi connection
// Task2 control timer when machine turn on
void Task1code( void * pvParameters ) {
  while (1) {
    // Check button is pressed or not
    Button_ByPass();

    if(paksaNyala){
      setMachineON = false;
      machineOn = false;
      killTransaction = true;

      updateServer = true;

      currentTime = 0;
      prevTime = 0;

      paksaNyala = false;
    }
    
    // setMachineOn will be true if status machine in the server change from 0 to 1, it means the controller must activate this Wassher/Dryer
    if (setMachineON) {
      MACHINE_on();

      currentTime = millis();
      if ((currentTime - prevTime) >= 1900) {
        detik++;
        if (detik == 30) {
          menit++;
          detik = 0;
        }

        // If the timer reach TON_MACHINE : Washer/Dryer is finished
        if (menit == TON_MACHINE) {
          updateServer = true;
          setMachineON = false;
        }
        #ifdef DEBUG
          Serial.print("Menit : ");
          Serial.println(menit);
        #endif
        prevTime = millis();
      }
    }
    vTaskDelay(100);
  }
}

void loop() {
  ArduinoOTA.handle();
  
  // Check this controller is connected to Access Point or not
  // If not connect, Turn off led indicator and reconnect to WIFI until connected
  if (WiFi.status() != WL_CONNECTED) {
    #ifdef DEBUG
      Serial.println("WIFI : Reconnecting");
    #endif
    digitalWrite(LED_WIFI, LOW);
    digitalWrite(2, LOW);
    WIFI_Pairing();
  }
  // If this module connected to WiFi, get update machine_status and is_packet from server
  // and then trigger the machine if status machine change from 0 to 1
  else {
    // Get update machine_status and is_packet from server
    URL_Server = (String) URL + (String) GET_MACHINE + (String) MACHINE_ID;
    SERVER_getJsonResponse(URL_Server, "machine_status");

    // next : GET time machine
    
    // Update status Washer/Dryer to server after Washer/Dryer finished
    if (updateServer) {   
      //Change machine_status to false if timer reach value
      URL_Server = (String) URL + (String) GET_MACHINE + (String) MACHINE_ID;
      DB_Message = "{\"machine_status\":false}";
      if (SERVER_Update(URL_Server, DB_Message)) {
        #ifdef DEBUG
          Serial.println("Success Update machine_status:false on Server");
        #endif
        if(Transaction_ID == ""){
          if(packet) PACKET = "true";
          else PACKET = "false";
          URL_Server = (String)URL + (String)GET_ID + (String)PACKET + (String)GET_ID_2 + (String)MACHINE_ID + (String)GET_ID_3 + (String)STORE;
          Serial.println(URL_Server);
          SERVER_getJsonResponse(URL_Server, "_id");
        }
        else{
          URL_Server = (String) URL + (String) UPDATE_TRANSACTION + (String) Transaction_ID;
          if(killTransaction){
//            URL_Server = (String) URL + (String) UPDATE_TRANSACTION + (String) Transaction_ID;
            DB_Message = "{\"step_one\":true,\"transaction_finish\":true}";
            if(SERVER_Update(URL_Server, DB_Message)){
              menit = 0;
              detik = 0;
              updateServer = false;
              updateEEPROM = true;
              killTransaction = false;
            }
          }
          else{
            if(packet){
              // For washer only, switch transaction from washer to dryer
              if((MACHINE_ID == "1") || (MACHINE_ID == "3") || (MACHINE_ID == "5")){
//                URL_Server = (String) URL + (String) UPDATE_TRANSACTION + (String) Transaction_ID;
                DB_Message = "{\"step_one\":true}";
                if(SERVER_Update(URL_Server, DB_Message)){
                  menit = 0;
                  detik = 0;
                  updateServer = false;
                  updateEEPROM = true;
                }
              }
              // For dryer only, update transaction status from false to true
              else if((MACHINE_ID == "2") || (MACHINE_ID == "4") || (MACHINE_ID == "6")){
//                URL_Server = (String) URL + (String) UPDATE_TRANSACTION + (String) Transaction_ID;
                DB_Message = "{\"transaction_finish\":true}";
                if(SERVER_Update(URL_Server, DB_Message)){
                  menit = 0;
                  detik = 0;
                  updateServer = false;
                  updateEEPROM = true;
                }
              }
            }
            else {
//              URL_Server = (String) URL + (String) UPDATE_TRANSACTION + (String) Transaction_ID;
              DB_Message = "{\"transaction_finish\":true}";
              if(SERVER_Update(URL_Server, DB_Message)){
                menit = 0;
                detik = 0;
                updateServer = false;
                updateEEPROM = true;
              }
            }
          }
        }

        if(updateEEPROM){
          // Update data machine(Machine State = 0, Minute = 0) to EEPROM
          EEPROM.write(STS_ADDR, 0);
          EEPROM.write(MINUTE_ADDR, 0);
          EEPROM.commit();
          updateEEPROM = false;
        }
      }
    }
  }
}
