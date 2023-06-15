#pragma once
#include <Arduino.h>
#include <NRFLite.h>
#include "config.h"
#include "power.h"


enum class MSG_TYPES : uint8_t
{
    INIT,
    BOOT,
    OK,
    SET,
    GET,
    RESET,
    STATUS,
};



// Must be max 32 bytes
class ClientPacket
{
private:
    uint8_t ID;
    uint8_t UUID[4] = DEVICE_UUID;
    uint8_t MSG_TYPE;
    uint8_t FIRMWARE = FIRMWARE_VERSION;
    uint8_t POWER_TYPE = DEVICE_BATTERY_POWERED;
    uint8_t DATA[24]; // DATA contains two checksum bytes

    // Internal Variables, not included into the send Packet
    uint8_t dataSize;

    void addChecksum()
    {
        // Calculate the checksum by summing all the bytes
        uint16_t sum = 0;
        for (size_t i = 0; i < 32 - sizeof(DATA) + dataSize; i++)
        {
            sum += reinterpret_cast<const uint8_t *>(this)[i];
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
            Serial.println("Too large Client-dataSize!");
            return;
        }

        // Copy data into DATA
        memcpy(DATA, data, dataSize);
        MSG_TYPE = (uint8_t)type;
        this->dataSize = dataSize;
        ID = id;
        if (DEVICE_BATTERY_POWERED == 1){
            POWER_TYPE = batteryLevel(); 
        }
        addChecksum();
    }

    uint8_t getSize()
    {
        return (32 - sizeof(DATA) + dataSize + 2);
    }

    void print()
    {
        Serial.print("ClientPacket: ");
        Serial.print("ID: ");
        Serial.print(ID);
        Serial.print(" ");
        Serial.print("UUID: ");
        Serial.print(UUID[0]);
        Serial.print(" ");
        Serial.print(UUID[1]);
        Serial.print(" ");
        Serial.print(UUID[2]);
        Serial.print(" ");
        Serial.print(UUID[3]);
        Serial.print(" ");
        Serial.print("TYPE: ");
        Serial.print(MSG_TYPE);
        Serial.print(" ");
        Serial.print("FIRMWARE: ");
        Serial.print(FIRMWARE_VERSION);
        Serial.print(" ");
        Serial.print("POWER: ");
        Serial.print(POWER_TYPE);
        Serial.print(" ");
        Serial.print("DATA: ");
        for (int i = 0; i < dataSize + 2; i++)
        {
            Serial.print(DATA[i]);
            Serial.print(" ");
        }
        Serial.println();
    }
};

// Must be max 32 bytes
class ServerPacket
{
private:
    uint8_t ID;
    uint8_t UUID[4];
    uint8_t MSG_TYPE;
    uint8_t DATA[26]; // DATA contains two checksum bytes

    // Internal Variables, not included in the received Packet
    uint8_t dataSize;
    bool valid = false;

    bool checkChecksum()
    {
        // Calculate the checksum by summing all the bytes
        uint16_t sum = 0;
        for (size_t i = 0; i < 32 - sizeof(DATA) + dataSize; i++)
        {
            sum += reinterpret_cast<const uint8_t *>(this)[i];
        }

        uint16_t received_checksum = ((uint16_t)DATA[dataSize] << 8) | DATA[dataSize + 1];
        return sum == received_checksum;
    }

public:
    ServerPacket(uint8_t *pckData, uint8_t pckSize)
    {
        dataSize = pckSize - (32 - sizeof(DATA)) - 2;
        if (dataSize > sizeof(DATA) - 2)
        {
            Serial.println("Too large Server-dataSize!");
            return;
        }
        if (dataSize < 0)
        {
            Serial.println("Too small Server-dataSize!");
            return;
        }

        ID = pckData[0];
        memcpy(UUID, pckData + 1, 4);
        MSG_TYPE = pckData[5];
        memcpy(DATA, pckData + 6, dataSize + 2);
        valid = checkChecksum();
        //print();
    }

    bool isValid()
    {
        return valid;
    }

    uint8_t getID()
    {
        return ID;
    }

    uint8_t *getUUID()
    {
        return UUID;
    }

    uint8_t getTYPE()
    {
        return MSG_TYPE;
    }

    uint8_t *getDATA()
    {
        return DATA;
    }

    uint8_t getSize()
    {
        return dataSize;
    }

    void print()
    {
        Serial.print("ServerPacket: ");
        Serial.print("ID: ");
        Serial.print(ID);
        Serial.print(" ");
        Serial.print("UUID: ");
        Serial.print(UUID[0]);
        Serial.print(" ");
        Serial.print(UUID[1]);
        Serial.print(" ");
        Serial.print(UUID[2]);
        Serial.print(" ");
        Serial.print(UUID[3]);
        Serial.print(" ");
        Serial.print("TYPE: ");
        Serial.print(MSG_TYPE);
        Serial.print(" ");
        Serial.print("DATA: ");
        for (int i = 0; i < dataSize + 2; i++)
        {
            Serial.print(DATA[i]);
            Serial.print(" ");
        }
        Serial.println();
    }
};

extern bool serverConnected;
extern NRFLite _radio;

void resetEEPROM();
void printEEPROM(int n = 0);
void loadFromEEPROM();

void radioInit();
void connectToServer();
void sendStatus();