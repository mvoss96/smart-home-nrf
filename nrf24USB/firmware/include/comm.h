#pragma once
#include "pins.h"

bool testConnection(uint8_t channel, uint8_t address);
void sendStringMessage(const char *message, MSG_TYPES type);
void sendInitMessage();
void nrfListen();
bool nrfSend(uint8_t destination, void *data, uint8_t length, bool requireAck = true);
uint8_t readAckPayload();
int readDataFromSerial(uint8_t *buffer, uint8_t bufferSize);