#include <Arduino.h>
#include "RFcomm.h"
#include "power.h"
#include "util.h"

void setPinModes()
{
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
}

void setup()
{
  Serial.begin(115200);
  printGreetingMessage();
  printPowerStatus();

  setPinModes();
  delay(500);
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, HIGH);
  connectToServer();
  loadStatusFromEEPROM();
  sendStatus();
  printEEPROM(5);
  delay(500);
}

void loop()
{
  // Check for reset button
  if (!serverConnected)
  {
    Serial.println(F("Connecting to Server:"));
    connectToServer();
  }
  listenForPackets();
}
