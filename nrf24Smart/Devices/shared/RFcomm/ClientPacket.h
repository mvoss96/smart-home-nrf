#pragma once
#include <Arduino.h>
#include "RFmessages.h"
#include "config.h"
#include "power.h"


// Must be max 32 bytes
class ClientPacket
{
private:
    static uint8_t msgNum;
    uint8_t ID;
    uint8_t UUID[4] = DEVICE_UUID;
    uint8_t MSG_TYPE = (uint8_t)MSG_TYPES::ERROR;
    uint8_t FIRMWARE = FIRMWARE_VERSION;
    uint8_t POWER_TYPE = DEVICE_BATTERY_POWERED;
    uint8_t STATUS_INTERVAL = 0;
    uint8_t MSG_NUM = msgNum++;
    uint8_t DATA[22] = {0}; // DATA contains two checksum bytes

    // Internal Variables, not included into the send Packet
    size_t dataSize = 0;
    bool isInitialized = false;

    void addChecksum()
    {
        // Calculate the checksum by summing all the bytes
        uint16_t sum = 0;

        sum += ID;
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            sum += UUID[i];
        }
        sum += MSG_TYPE;
        sum += FIRMWARE;
        sum += POWER_TYPE;
        sum += STATUS_INTERVAL;
        sum += MSG_NUM;
        for (size_t i = 0; i < dataSize; i++)
        {
            sum += DATA[i];
        }

        // Store the checksum in DATA
        DATA[dataSize] = (sum >> 8) & 0xFF; // MSB
        DATA[dataSize + 1] = sum & 0xFF;    // LSB
    }

public:
    ClientPacket(uint8_t id, MSG_TYPES type, uint8_t *data, uint8_t dataSize)
    {
        if (dataSize > sizeof(DATA) - 2)
        {
            Serial.println(F("Too large Client-dataSize!"));
            return;
        }

        // Copy data into DATA
        memcpy(DATA, data, dataSize);
        MSG_TYPE = (uint8_t)type;
        this->dataSize = dataSize;
        ID = id;
        STATUS_INTERVAL = statusInterval;
        if (DEVICE_BATTERY_POWERED == 1)
        {
            POWER_TYPE = batteryLevel();
        }
        addChecksum();
        isInitialized = true;
    }

    size_t getSize()
    {
        if (!isInitialized)
        {
            return 0;
        }
        return (32 - sizeof(DATA) + dataSize + 2);
    }

    bool getInitialized()
    {
        return isInitialized;
    }

    void printData()
    {
        Serial.print(F(": ["));
        size_t s = getSize();
        for (size_t i = 0; i < s; i++)
        {
            Serial.print((int)(((uint8_t *)this)[i]), HEX);
            if (i < s - 1)
            {
                Serial.print(F(" "));
            }
        }
        Serial.print(F("] "));
    }

    void print()
    {
        if (!isInitialized)
        {
            Serial.println(F("Attempted to print an uninitialized ClientPacket!"));
            return;
        }
        Serial.print(F("ClientPacket: "));
        Serial.print(F("ID: "));
        Serial.print(ID);
        Serial.print(F(" "));
        Serial.print(F("UUID: "));
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            Serial.print(UUID[i]);
            Serial.print(F(" "));
        }
        Serial.print(F("TYPE: "));
        Serial.print(MSG_TYPE);
        Serial.print(F(" "));
        Serial.print(F("FIRMWARE: "));
        Serial.print(FIRMWARE_VERSION);
        Serial.print(F(" "));
        Serial.print(F("POWER: "));
        Serial.print(POWER_TYPE);
        Serial.print(F(" "));
        Serial.print(F("STATUS_INTERVAL: "));
        Serial.print(STATUS_INTERVAL);
        Serial.print(F(" "));
        Serial.print(F("MSG_NUM: "));
        Serial.print(MSG_NUM);
        Serial.print(F(" "));
        Serial.print(F("DATA: "));
        for (size_t i = 0; i < dataSize + 2; i++)
        {
            Serial.print(DATA[i]);
            Serial.print(F(" "));
        }
        // Serial.println();
    }
};