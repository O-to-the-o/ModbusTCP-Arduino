// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to ModbusClient
//               MIT license - see license.md for details
// =================================================================================================

// Example code to show the usage of the eModbus library. 
// Please refer to root/Readme.md for a full description.

// Includes: <Arduino.h> for Serial etc., WiFi.h for WiFi support
#include <Arduino.h>
#include <ESP8266WiFi.h> //Wichtig, die ESPWiFi Libary ist nicht Kompatibel mit dem ESP8266
#include "credentials.h" //Beispiel in beispielWlan.h

// Include the header for the ModbusClient TCP style
#include <ModbusClientTCPasync.h>

char ssid[] = MY_SSID;                     // SSID and ...
char pass[] = MY_PASS;                     // password for the WiFi network used
ModbusClientTCPasync MB(ip, port);
int sensorPins[]={
  5,
  13,
  12,
  14,
  16};
uint sensorAnzahl=sizeof(sensorPins)/sizeof(sensorPins[0]);
// Define an onData handler function to receive the regular responses
// Arguments are Modbus server ID, the function code requested, the message data and length of it, 
// plus a user-supplied token to identify the causing request
void handleData(ModbusMessage response, uint32_t token) 
{
  digitalWrite(LED_BUILTIN,1); //Led aus wenn kein Fehler vorlieg
  Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
  for (auto& byte : response) {
    Serial.printf("%02X ", byte);
  }
  Serial.println("");
}

// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token) 
{
  digitalWrite(LED_BUILTIN,0); // Led an wenn Fehler
  // ModbusError wraps the error code and provides a readable error message for it
  ModbusError me(error);
  Serial.printf("Error response: %02X - %s token: %d\n", (int)me, (const char *)me, token);
}

// Setup() - initialization happens here
void setup() {
  //Pindefinition des Sensor Arrays
  for(uint i=0; i<sensorAnzahl;i++){
    pinMode(sensorPins[i],INPUT);
  };
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("__ OK __");
// Connect to WiFi

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(MY_SSID,MY_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
    Serial.print(". ");
    delay(1000);
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  IPAddress wIP = WiFi.localIP();
  Serial.printf("WIFi IP address: %u.%u.%u.%u\n", wIP[0], wIP[1], wIP[2], wIP[3]);
// Set up ModbusTCP client.
// - provide onData handler function
  MB.onDataHandler(&handleData);
// - provide onError handler function
  MB.onErrorHandler(&handleError);
// Set message timeout to 2000ms and interval between requests to the same host to 200ms
  MB.setTimeout(10000);
// Start ModbusTCP background task
  MB.setIdleTimeout(60000);
}

// loop() - nothing done here today!
void loop() {
  //digitalWrite(LED_BUILTIN,1);
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 5000) { //nach mindestens 5s die Sensoren einlesen und Nachricht an Modbus Client absetzen
    lastMillis = millis();
    bool sensorenZustand[sensorAnzahl];
    for(uint i=0; i<sensorAnzahl;i++){
      sensorenZustand[i]=digitalRead(sensorPins[i])>0?true:false;
    }
    int modbusNachricht=0;
    //Zusammenbauen des Nachrichtenwertes durch Bitshifting, in der Wago sind die einzelnen Bits wie Eingänge auswertbar
    for(uint i = 0;i<sensorAnzahl;i++){ //Sizeoff gibt die Arraylänge in bytes zurück (Bool ist 1 Byte)
      modbusNachricht=(modbusNachricht<<1)+(sensorenZustand[i]);
    }
    //Ausgabe des Arrays und anschließend der Nachricht zur Kontrolle
    Serial.println("------Sensoren Zustand------");
    for(uint i = 0;i<sensorAnzahl;i++){
      Serial.printf("%u ",sensorenZustand[i]);
    }
    Serial.println("\n------Modbus Nachricht------");
    Serial.println(modbusNachricht);
    Serial.printf("sending request with token %d\n", (uint32_t)lastMillis);
    Error err;
    //1.Identifer für die Nachricht rein vom Nutzer Festgelegt, 2.Server ID, 3.Funktions ID, 4.Addresse, ....
    err = MB.addRequest((uint32_t)lastMillis, 1, WRITE_HOLD_REGISTER, 1,modbusNachricht);
    if (err != SUCCESS) {
      ModbusError e(err);
      Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    }
    // Else the request is processed in the background task and the onData/onError handler functions will get the result.
    //
    // The output on the Serial Monitor will be (depending on your WiFi and Modbus the data will be different):
    //     __ OK __
    //     . WIFi IP address: 192.168.178.74
    //     Response: serverID=20, FC=3, Token=0000056C, length=11:
    //     14 03 04 01 F6 FF FF FF 00 C0 A8
  }
}