#pragma once
#include "config.h"

// different blink patters
enum BlinkCode
{
  BLINK_INIT_OK,
  BLINK_INIT_ERROR,
  BLINK_ONCE,
};

#pragma once

void blink(uint8_t pin, uint8_t count, unsigned long duration);
void blinkCode(uint8_t blink_pattern);