#pragma once
#include <Arduino.h>

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