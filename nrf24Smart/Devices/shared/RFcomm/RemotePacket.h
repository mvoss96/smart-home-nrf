#pragma once
#include <Arduino.h>

class FromRemotePacket
{
private:
    // Internal Variables, not included in the received Packet
    bool valid = false;

public:
    uint8_t ID;
    uint8_t UUID[4] = DEVICE_UUID;
    uint8_t MESSAGE_TYPE = (uint8_t)MSG_TYPES::REMOTE;
    uint8_t TARGET_UUID[4];
    uint8_t LAYER;
    uint8_t VALUE;
    uint8_t CHECKSUM[2];

    FromRemotePacket(uint8_t *pckData, uint8_t pckSize)
    {
        if (pckSize < 14)
        {
            Serial.println(F("Too small pckSize"));
            return;
        }
        if (pckSize > 32)
        {
            Serial.println(F("Too large pckSize"));
            return;
        }

        ID = pckData[0];
        memcpy(UUID, pckData + 1, 4);
        MESSAGE_TYPE = pckData[5];
        memcpy(TARGET_UUID, pckData + 6, 4);
        LAYER = pckData[10];
        VALUE = pckData[11];
        CHECKSUM[0] = pckData[12];
        CHECKSUM[1] = pckData[13];
        valid = checkChecksum();
    }

    bool checkChecksum()
    {
        // Calculate the checksum by summing all the bytes
        uint16_t sum = 0;

        sum += ID;
        for (size_t i = 0; i < 4; i++)
        {
            sum += UUID[i];
        }
        sum += MESSAGE_TYPE;
        for (size_t i = 0; i < 4; i++)
        {
            sum += TARGET_UUID[i];
        }
        sum += LAYER;
        sum += VALUE;

        uint16_t received_checksum = ((uint16_t)CHECKSUM[0] << 8) | CHECKSUM[1];
        return sum == received_checksum;
    }

    bool isValid()
    {
        return valid;
    }

    void printData()
    {
        Serial.print(F(": ["));
        Serial.print(F("] "));
    }

    void print()
    {
        Serial.print(F("FromRemotePacket: "));
        Serial.print(F("valid: "));
        Serial.print(valid);
        Serial.print(F(" "));
        Serial.print(F("ID: "));
        Serial.print(ID);
        Serial.print(F(" "));
        Serial.print(F("UUID: "));
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            Serial.print(UUID[i]);
            Serial.print(" ");
        }
        Serial.print(F("TARGET_UUID: "));
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            Serial.print(TARGET_UUID[i]);
            Serial.print(" ");
        }
        Serial.print(F("LAYER: "));
        Serial.print(LAYER);
        Serial.print(F(" "));
        Serial.print(F("VALUE: "));
        Serial.print(VALUE);
        Serial.print(F(" "));
        Serial.print(F("CHECKSUM: "));
        Serial.print(((uint16_t)CHECKSUM[0] << 8) | CHECKSUM[1]);
    }
};

// Must be max 32 bytes
class RemotePacket
{
private:
    uint8_t ID;
    uint8_t UUID[4] = DEVICE_UUID;
    uint8_t MESSAGE_TYPE = (uint8_t)MSG_TYPES::REMOTE;
    uint8_t TARGET_UUID[4];
    uint8_t LAYER;
    uint8_t VALUE;
    uint8_t CHECKSUM[2];

    void addChecksum()
    {
        // Calculate the checksum by summing all the bytes
        uint16_t sum = 0;

        sum += ID;
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            sum += UUID[i];
        }
        sum += MESSAGE_TYPE;
        for (size_t i = 0; i < sizeof(TARGET_UUID); i++)
        {
            sum += TARGET_UUID[i];
        }
        sum += LAYER;
        sum += VALUE;

        // Store the checksum
        CHECKSUM[0] = (sum >> 8) & 0xFF; // MSB
        CHECKSUM[1] = sum & 0xFF;        // LSB
    }

public:
    RemotePacket(uint8_t id, uint8_t *targetUUID, LAYERS layer, uint8_t value)
    {
        ID = id;
        memcpy(TARGET_UUID, targetUUID, 4);
        LAYER = (uint8_t)layer;
        VALUE = value;
        addChecksum();
    }

    size_t getSize()
    {
        return (sizeof(*this));
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
                Serial.print(" ");
            }
        }
        Serial.print(F("] "));
    }

    void print()
    {
        Serial.print(F("RemotePacket: "));
        Serial.print(F("ID: "));
        Serial.print(ID);
        Serial.print(F(" "));
        Serial.print(F("UUID: "));
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            Serial.print(UUID[i]);
            Serial.print(F(" "));
        }
        Serial.print(F("TARGET_UUID: "));
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            Serial.print(TARGET_UUID[i]);
            Serial.print(F(" "));
        }
        Serial.print(F("LAYER: "));
        Serial.print(LAYER);
        Serial.print(F(" "));
        Serial.print(F("VALUE: "));
        Serial.print(VALUE);
        Serial.print(F(" "));
        Serial.print(F("CHECKSUM: "));
        Serial.print(((uint16_t)CHECKSUM[0] << 8) | CHECKSUM[1]);
    }
};