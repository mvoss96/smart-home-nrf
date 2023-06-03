#include <Arduino.h>
#include "pins.h"
#include "comm.h"
#include "blinkcodes.h"

const uint8_t BUFFER_SIZE = 64;
uint8_t buffer[BUFFER_SIZE];

void setup()
{
  // Put your setup code here, to run once:
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  Serial.begin(SERIAL_SPEED);
  long lastInitMsg = -10000;

  while (true)
  {
    if (millis() - lastInitMsg > 500)
    {
      sendInitMessage();
      lastInitMsg = millis();
    }
    delay(10); // Short delay to let the host read the message
    int packetSize = readDataFromSerial(buffer, BUFFER_SIZE);
    if (packetSize == 2) // Check if a correct packet was successfully received
    {
      MSG_TYPES receivedType = static_cast<MSG_TYPES>(buffer[0]); // Extract the packet type
      if (receivedType == MSG_TYPES::INIT)                        // Check if the packet type is INIT
      {
        uint8_t channel = buffer[1]; // Byte at position 1 is first data byte since byte 0 is msg_type
        uint8_t address = buffer[2];
        if (!testConnection(channel, address))
        {
          for (;;)
          {
            sendStringMessage("NRF ERROR!", MSG_TYPES::ERROR);
            delay(1000);
          }
        }
        char msg[32];
        snprintf(msg, sizeof(msg), "NRF INIT channel:%d address:%d", channel, address);
        sendStringMessage(msg, MSG_TYPES::OK);
        break;
      }
    }
  }
  // testConnection(100, 1);
}

void loop()
{
  // Check if a serial message was received
  int packetSize = readDataFromSerial(buffer, BUFFER_SIZE);
  if (packetSize > 0)
  {
    MSG_TYPES receivedType = static_cast<MSG_TYPES>(buffer[0]); // Extract the packet type
    if (receivedType == MSG_TYPES::REBOOT)
    {
      sendStringMessage("REBOOT...", MSG_TYPES::REBOOT);
      delay(100);
      asm("JMP 0");
    }
    if (receivedType == MSG_TYPES::MSG)
    {
      uint8_t destinationID = buffer[1];
      uint8_t requireAck = buffer[2];
      // char msg[32];
      // snprintf(msg, sizeof(msg), "send to:%d ack:%d", destinationID, requireAck);
      // sendStringMessage(msg, MSG_TYPES::INFO);
      nrfSend(destinationID, buffer + 3, packetSize - 2, static_cast<bool>(requireAck));
    }
  }

  // uint8_t msg[] = {1,0,2};
  // nrfSend(0, msg, 3, true);
  // blinkCode(BLINK_ONCE);
  // delay(100);
  // Listen for nrf messages
  
  nrfListen();
}