#pragma once
#include <Arduino.h>
#include "config.h"

struct Status
{
    uint8_t targetID = 0;
    uint8_t targetUUID[4] = {0, 0, 0, 0};
    float   temperature = -999.0;
    float   humidity    = -999.0;

};

extern Status status;
