#include <Arduino.h>
#include "RFcomm.h"
#include "power.h"
#include "util.h"
#include "control.h"

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
  pinMode(PIN_BTN_LISTEN, INPUT_PULLUP);

  // Enable interrupt on PIN_BTN1
  PCICR |= (1 << PCIE1);   // Aktivate Interrupts on Port C (analog)
  PCMSK1 |= (1 << PCINT4); // Aktivate Pin Change ISR for A4
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
  printEEPROM(12);
  setupEncoder();
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
  checkForSleep();
  // Check for reset button
  if (btnPressed)
  {
    btnPressed = false;
    resetEEPROM();
    delay(1000);
  }

  if (!serverConnected)
  {
    Serial.println(F("Connecting to Server:"));
    connectToServer();
  }

  // Listen for packets while Listen Button pressed
  if (digitalRead(PIN_BTN_LISTEN) == LOW)
  {
    digitalWrite(PIN_LED1, LOW);
    checkForSleep(); // Wake Up Radio
    sendStatus();
    while (digitalRead(PIN_BTN_LISTEN) == LOW)
    {
      listenForPackets();
    }
    digitalWrite(PIN_LED1, HIGH);
  }

  readEncoder();
  sendEvents();
}
