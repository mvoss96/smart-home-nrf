#include <Arduino.h>
#include "RFcomm.h"
#include "blink.h"
#include "power.h"

void (*resetFunc)(void) = 0;
volatile bool btnPressed = false;

ISR(PCINT1_vect)
{
  btnPressed = !digitalRead(PIN_BTN1);
}

void setPinModes()
{
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_OUTPUT_R, OUTPUT);
  pinMode(PIN_OUTPUT_G, OUTPUT);
  pinMode(PIN_OUTPUT_B, OUTPUT);
  pinMode(PIN_BTN1, INPUT_PULLUP);

  // Enable interrupt on PIN_BTN1
  PCICR |= (1 << PCIE1);   // Aktivate Interrupts on Port C (analog)
  PCMSK1 |= (1 << PCINT4); // Aktivate Pin Change ISR for A4
}

void setup()
{
  Serial.begin(115200);
  Serial.println(DEVICE_TYPE);
  Serial.println(readVcc());
  Serial.println(batteryLevel());
  setPinModes();
  delay(500);
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, HIGH);
  connectToServer();
  printEEPROM(5);
  delay(1000);
}

void loop()
{
  // Check for reset button
  if (btnPressed)
  {
    btnPressed = false;
    resetEEPROM();
    delay(1000);
  }

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
      blink(PIN_LED2, 1, 100);
      switch ((MSG_TYPES)pck.getTYPE())
      {
      case MSG_TYPES::RESET:
        Serial.println("RESET message received: resetting Device...");
        resetEEPROM();
        break;
      case MSG_TYPES::GET:
        Serial.println("GET message received: sending status...");
        sendStatus();
        break;
      default:
        Serial.println("Unsupported message received!");
      }
    }
  }
}
