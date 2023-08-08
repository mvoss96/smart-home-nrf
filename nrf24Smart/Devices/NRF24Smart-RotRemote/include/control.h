#pragma once
#include <Arduino.h>
#include "config.h"

struct Status{
    uint8_t test = 0;
};

extern Status status;

void setStatus(const uint8_t* data, uint8_t length);

