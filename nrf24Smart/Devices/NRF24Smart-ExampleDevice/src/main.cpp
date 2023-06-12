#include <Arduino.h>
#include "RFcomm.h"

void (*resetFunc)(void) = 0;

void setup()
{
  Serial.begin(115200);
  resetEEPROM();
  delay(100000000);
  connectToServer();
  printEEPROM(5);
  delay(1000);
}

void loop()
{
  if (!serverConnected)
  {
    Serial.println("Connecting to Server:");
    connectToServer();
  }
  // Listen for new Messages
  uint8_t packetSize = _radio.hasData();
  uint8_t buf[32] = {0};
  if (packetSize > 0)
  {
    _radio.readData(&buf);
    ServerPacket pck(buf, packetSize);
    if (pck.isValid())
    {
      switch ((MSG_TYPES)pck.getTYPE())
      {
      case MSG_TYPES::RESET:
        Serial.println("RESET message received: resetting Device...");
        resetEEPROM();
        break;
      default:
        Serial.println("Unsupported message received!");
      }
    }
  }
}
