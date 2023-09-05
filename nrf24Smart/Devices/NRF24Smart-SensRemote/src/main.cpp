#include <Arduino.h>
#include "RFcomm.h"
#include "power.h"
#include "util.h"
#include "control.h"

volatile bool btnPressed = false;

void setPinModes()
{
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_RST, INPUT_PULLUP);
  pinMode(PIN_LISTEN, INPUT_PULLUP);
  pinMode(PIN_BTN1_UP, INPUT_PULLUP);
  pinMode(PIN_BTN1_DN, INPUT_PULLUP);

  // Enable interrupt on pins 5 and 6 (BTN1_UP and BTN1_DN)
  PCICR |= (1 << PCIE2);                     // Enable Pin Change Interrupts on PORTD
  PCMSK2 |= (1 << PCINT21) | (1 << PCINT22); // Enable Pin Change ISR for PD5 and PD6

  // Enable Pin Change Interrupts on PORTC (for pins A3 and A0)
  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT11) | (1 << PCINT8);
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
  readSensor();
  sendStatus();
  printEEPROM(12);
  delay(500);

  // pin change interrupt (example for D4 and D5)
  noInterrupts();
  PCMSK2 |= bit(PCINT20); // enable pin change interrupt for D4
  PCMSK2 |= bit(PCINT21); // enable pin change interrupt for D5
  PCIFR |= bit(PCIF2);    // clear any outstanding interrupts
  PCICR |= bit(PCIE2);    // enable pin change interrupts for D0 to D7
  interrupts();
}

void loop()
{
  // Check for reset
  if (digitalRead(PIN_RST) == LOW)
  {
    resetEEPROM();
    delay(1000);
  }

  // Check for Reprogram
  if (digitalRead(PIN_LISTEN) == LOW)
  {
    digitalWrite(PIN_LED1, LOW);
    while (digitalRead(PIN_LISTEN) == LOW)
    {
      listenForPackets();
    }
    digitalWrite(PIN_LED1, HIGH);
  }

  checkForSleep();
  readButtons();
  sendEvents();

  if (!serverConnected)
  {
    Serial.println(F("Connecting to Server:"));
    connectToServer();
  }
}
