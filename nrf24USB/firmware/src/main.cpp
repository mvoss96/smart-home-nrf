#include "config.h"
#include "rfComm.h"
#include "serialComm.h"

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

  waitForHost();
}

void loop()
{
  static uint8_t packetSize = 0;
  static uint8_t buf[32];
  
  if (nrfListen(buf, packetSize))
  {
    sendSerialPacket(buf, packetSize, MSG_TYPES::MSG);
  }

  // Check if a serial message was received
  checkForSerialMsg();
}