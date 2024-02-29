#pragma once
#include <Arduino.h>

void printGreetingMessage()
{
  Serial.println(DEVICE_TYPE);
}

#ifdef __AVR__
#include <avr/wdt.h>

void softwareReset()
{
  // enable the wdt for the shortest possible setting
  wdt_enable(WDTO_15MS);
  // wait for the prescaller time to expire
  // without sending the reset signal by using
  // the wdt_reset() method
  while (1)
  {
  }
}
#endif