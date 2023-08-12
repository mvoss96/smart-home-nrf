#pragma once
#include <Arduino.h>
#include "config.h"

struct Status
{
    uint8_t test1 = 1;
    uint8_t test2 = 2;
    uint8_t test3 = 3;
};

extern Status status;
