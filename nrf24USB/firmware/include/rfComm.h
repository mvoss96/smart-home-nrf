#pragma once
#include "config.h"

bool nrfListen(uint8_t *data, uint8_t &packetSize);
bool radioInit(uint8_t channel, uint8_t address);
bool nrfSend(uint8_t destination, void *data, uint8_t length, bool requireAck = true);

extern bool blinkOnMessage;