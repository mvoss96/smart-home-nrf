#include <Arduino.h>
#include "RFcomm.h"
#include "power.h"
#include "util.h"

void (*resetFunc)(void) = 0;
volatile bool btnPressed = false;

ISR(PCINT1_vect)
{
  btnPressed = !digitalRead(PIN_BTN_RST);
}

void setPinModes()
{
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);

  pinMode(PIN_BTN_RST, INPUT_PULLUP);
  pinMode(PIN_BTN_ENC, INPUT_PULLUP);
  pinMode(PIN_ROTARY1, INPUT_PULLUP);
  pinMode(PIN_ROTARY2, INPUT_PULLUP);
  pinMode(PIN_RADIO_CE, OUTPUT);

  // Enable interrupt on PIN_BTN1
  PCICR |= (1 << PCIE1);   // Aktivate Interrupts on Port C (analog)
  PCMSK1 |= (1 << PCINT4); // Aktivate Pin Change ISR for A4
}

void setup()
{
  Serial.begin(115200);
  printGreetingMessage();
  Serial.print("VCC: ");
  Serial.print(readVcc());
  Serial.println("mV");
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
  listenForPackets();
}
