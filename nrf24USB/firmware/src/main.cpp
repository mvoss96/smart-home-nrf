#include <Arduino.h>
#include "pins.h"
#include "comm.h"
#include "blinkcodes.h"

// #define TEST_MODE

void setup()
{
  // Put your setup code here, to run once:
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED2, HIGH);
  digitalWrite(PIN_LED2, HIGH);
  Serial.begin(SERIAL_SPEED);
  Serial.println(DEVICE_NAME);
  Serial.flush();
  delay(100);

#ifdef TEST_MODE
  testConnection(101, 0);
#else
  waitForHost();
#endif
}

void loop()
{
  // Listen for nrf messages
  nrfListen();
  
  // Check if a serial message was received
  checkForSerialMsg();
}