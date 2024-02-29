#include <Arduino.h>
#include "RFcomm.h"
#include "blink.h"
#include "power.h"
#include "control.h"
#include "util.h"

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
  digitalWrite(PIN_OUTPUT_R, LOW);
  digitalWrite(PIN_OUTPUT_G, LOW);
  digitalWrite(PIN_OUTPUT_B, LOW);
  pinMode(PIN_BTN1, INPUT_PULLUP);

  // Enable interrupt on PIN_BTN1
  PCICR |= (1 << PCIE1);   // Aktivate Interrupts on Port C (analog)
  PCMSK1 |= (1 << PCINT4); // Aktivate Pin Change ISR for A4

#ifdef NRF_USE_IRQ
  attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);
#endif

  delay(500);
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, HIGH);
}

void setup()
{
  Serial.begin(115200);
  printGreetingMessage();
  setPinModes();
  loadDeviceSettingsFromEEPROM();
  loadServerSettingsFromEEPROM();
  connectToServer();
  delay(1000);
  setOutput();
}

void loop()
{
  // Check for reset button
  if (btnPressed)
  {
    btnPressed = false;
    resetServerSettingsFromEEPROM();
    Serial.flush();
    delay(1000);
    softwareReset();
  }

  listenForPackets();
}
