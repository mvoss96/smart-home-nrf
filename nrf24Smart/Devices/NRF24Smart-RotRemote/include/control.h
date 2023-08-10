#pragma once
#include <Arduino.h>
#include "config.h"

struct Status
{
    uint8_t targetID = 0;
    uint8_t targetUUID[4] = {0, 0, 0, 0};
};

extern Status status;

void setStatus(const uint8_t *data, uint8_t length);
void setupEncoder();
void readEncoder();
void checkForSleep();
