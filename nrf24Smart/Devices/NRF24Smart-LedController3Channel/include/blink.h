#pragma once
#include <Arduino.h>

void blink(uint8_t pin, uint8_t count, unsigned long duration);
void blinkBoth(uint8_t count, unsigned long duration);
long readVcc();