#pragma once
#include <Arduino.h>

// different blink patters
enum class BlinkCode
{
  BLINK_INIT_OK,
  BLINK_INIT_ERROR,
  BLINK_ONCE,
};

#pragma once

void blink(uint8_t pin, uint8_t count, unsigned long duration);
void blinkCode(BlinkCode blink_pattern);