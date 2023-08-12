#pragma once
#include <Arduino.h>

// Must be max 32 bytes
class ServerPacket
{
private:
    uint8_t ID;
    uint8_t UUID[4];
    uint8_t MSG_TYPE;
    uint8_t DATA[26]; // DATA contains two checksum bytes

    // Internal Variables, not included in the received Packet
    size_t dataSize;
    bool valid = false;

    bool checkChecksum()
    {
        // Check dataSize is within bounds
        if (dataSize > 24)
        {
            return false;
        }

        // Calculate the checksum by summing all the bytes
        uint16_t sum = 0;

        sum += ID;
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            sum += UUID[i];
        }
        sum += MSG_TYPE;
        for (size_t i = 0; i < dataSize; i++)
        {
            sum += DATA[i];
        }

        // Check if there are enough bytes for the checksum
        if (dataSize + 1 >= sizeof(DATA))
        {
            return false;
        }

        uint16_t received_checksum = ((uint16_t)DATA[dataSize] << 8) | DATA[dataSize + 1];
        return sum == received_checksum;
    }

public:
    ServerPacket(uint8_t *pckData, uint8_t pckSize)
    {
        if (pckSize < 6)
        {
            Serial.println(F("Too small pckSize"));
            return;
        }
        if (pckSize > 32)
        {
            Serial.println(F("Too large pckSize"));
            return;
        }

        dataSize = pckSize - (32 - sizeof(DATA)) - 2;
        if (dataSize > sizeof(DATA) - 2)
        {
            Serial.println(F("Too large Server-dataSize!"));
            return;
        }
        if (dataSize < 0)
        {
            Serial.println(F("Too small Server-dataSize!"));
            return;
        }

        ID = pckData[0];
        memcpy(UUID, pckData + 1, 4);
        MSG_TYPE = pckData[5];
        memcpy(DATA, pckData + 6, dataSize + 2);
        valid = checkChecksum();
    }

    bool isValid()
    {
        return valid;
    }

    uint8_t getID()
    {
        return ID;
    }

    const uint8_t *getUUID()
    {
        return UUID;
    }

    uint8_t getTYPE()
    {
        return MSG_TYPE;
    }

    const uint8_t *getDATA()
    {
        return DATA;
    }

    uint8_t getSize()
    {
        return dataSize;
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
        Serial.print(F("ServerPacket: "));
        Serial.print(F("ID: "));
        Serial.print(ID);
        Serial.print(F(" "));
        Serial.print(F("UUID: "));
        Serial.print(UUID[0]);
        Serial.print(F(" "));
        Serial.print(UUID[1]);
        Serial.print(F(" "));
        Serial.print(UUID[2]);
        Serial.print(F(" "));
        Serial.print(UUID[3]);
        Serial.print(F(" "));
        Serial.print(F("TYPE: "));
        Serial.print(MSG_TYPE);
        Serial.print(F(" "));
        Serial.print(F("DATA: "));
        for (size_t i = 0; i < dataSize + 2; i++)
        {
            Serial.print(DATA[i]);
            Serial.print(F(" "));
        }
        Serial.println();
    }
};