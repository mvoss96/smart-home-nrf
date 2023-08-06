#include <Arduino.h>
#include "RFcomm.h"
#include "blink.h"
#include "power.h"
#include "control.h"

void (*resetFunc)(void) = 0;
volatile bool btnPressed = false;

ISR(PCINT1_vect)
{
  btnPressed = !digitalRead(PIN_BTN1);
}

void printGreetingMessage()
{
  static const uint8_t uuid[] = DEVICE_UUID;
  Serial.print(DEVICE_TYPE);
  Serial.print(" [");
  for (size_t i = 0; i < 4; i++)
  {
    Serial.print(uuid[i], HEX);
    if (i < 3)
    {
      Serial.print(" ");
    }
  }
  Serial.println("] ");
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
  printGreetingMessage();
  // Serial.println(readVcc());
  // Serial.println(batteryLevel());
  setPinModes();
  delay(500);
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, HIGH);
  connectToServer();
  printEEPROM(5);
  delay(1000);
  setOutput();
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
  listenForPackets();
}
