#pragma once
#include <Arduino.h>
#include <NRFLite.h>
#include "ClientPacket.h"
#include "ServerPacket.h"
#include "RemotePacket.h"


extern bool serverConnected;
extern NRFLite _radio;
extern uint8_t radioID;
extern uint8_t serverUUID[4];


void connectToServer();
void radioInit();

void listenForPackets();
void sendStatus(bool isAck);
void sendRemote(LAYERS layer, uint8_t value);

void resetEEPROM();
void saveToEEPROM();
void printEEPROM(int n = 0);
void loadFromEEPROM();
void loadStatusFromEEPROM();

inline void sendStatus()
{
    sendStatus(false);
}
