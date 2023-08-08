#pragma once
#include <Arduino.h>
#include "config.h"

struct Status{
    uint8_t power = true;
    uint8_t brightness = 127;
    uint8_t ch_1 = 255;
    uint8_t ch_2 = 255;
    uint8_t ch_3 = 255;
    uint8_t num_channels = NUM_LED_CHANNELS;
    float   powerScale = 1.0;
   
};

extern Status status;

void setStatus(const uint8_t* data, uint8_t length);
void setOutput();

