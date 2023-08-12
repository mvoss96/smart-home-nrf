#pragma once
#include "status.h"

void setStatus(const uint8_t *data, uint8_t length);
void setupEncoder();
void readEncoder();
void checkForSleep();
void sendEvents();
