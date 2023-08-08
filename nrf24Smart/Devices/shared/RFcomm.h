#pragma once
#include <Arduino.h>
#include <NRFLite.h>
#include "config.h"
#include "power.h"



enum class MSG_TYPES : uint8_t
{
    ERROR,
    INIT,
    BOOT,
    SET,
    RESET,
    STATUS,
};

extern uint8_t statusInterval;


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
    uint8_t STATUS_INTERVAL = STATUS_INTERVAL_TIME;
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
            Serial.println("Too large Client-dataSize!");
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
        Serial.print(": [");
        size_t s = getSize();
        for (size_t i = 0; i < s; i++)
        {
            Serial.print((int)(((uint8_t *)this)[i]), HEX);
            if (i < s - 1)
            {
                Serial.print(" ");
            }
        }
        Serial.print("] ");
    }

    void print()
    {
        if (!isInitialized)
        {
            Serial.println("Attempted to print an uninitialized ClientPacket!");
            return;
        }
        Serial.print("ClientPacket: ");
        Serial.print("ID: ");
        Serial.print(ID);
        Serial.print(" ");
        Serial.print("UUID: ");
        for (size_t i = 0; i < sizeof(UUID); i++)
        {
            Serial.print(UUID[i]);
            Serial.print(" ");
        }
        Serial.print("TYPE: ");
        Serial.print(MSG_TYPE);
        Serial.print(" ");
        Serial.print("FIRMWARE: ");
        Serial.print(FIRMWARE_VERSION);
        Serial.print(" ");
        Serial.print("POWER: ");
        Serial.print(POWER_TYPE);
        Serial.print(" ");
        Serial.print("STATUS_INTERVAL: ");
        Serial.print(STATUS_INTERVAL);
        Serial.print(" ");
        Serial.print("MSG_NUM: ");
        Serial.print(MSG_NUM);
        Serial.print(" ");
        Serial.print("DATA: ");
        for (size_t i = 0; i < dataSize + 2; i++)
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
            Serial.println("Too small pckSize");
            return;
        }
        if (pckSize > 32)
        {
            Serial.println("Too large pckSize");
            return;
        }

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
        Serial.print(": [");
        size_t s = getSize();
        for (size_t i = 0; i < s; i++)
        {
            Serial.print((int)(((uint8_t *)this)[i]), HEX);
            if (i < s - 1)
            {
                Serial.print(" ");
            }
        }
        Serial.print("] ");
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
        for (size_t i = 0; i < dataSize + 2; i++)
        {
            Serial.print(DATA[i]);
            Serial.print(" ");
        }
        Serial.println();
    }
};

enum class ChangeTypes : uint8_t
{
    INVALID,
    SET,
    TOGGLE,
    INCREASE,
    DECREASE,
};

struct SetMessage
{
    uint8_t varIndex = 0;
    ChangeTypes changeType = ChangeTypes::INVALID;
    uint8_t valueSize = 0;
    const uint8_t *newValue;

    bool isValid = false;

    SetMessage(const uint8_t *data, uint8_t size)
    {
        if (size < 4)
        {
            Serial.println("ERROR: Too small SetMessage size!");
            return;
        }
        varIndex = data[0];
        uint8_t rawChangeType = data[1];
        if (rawChangeType > static_cast<uint8_t>(ChangeTypes::DECREASE))
        {
            Serial.println("ERROR: Invalid changeType!");
            return;
        }
        changeType = static_cast<ChangeTypes>(rawChangeType);
        valueSize = data[2];
        if (size - valueSize != 3)
        {
            Serial.println("ERROR: Incompatible SetMessage size!");
            return;
        }
        newValue = data + 3;
        isValid = true;
    }
};

extern bool serverConnected;
extern NRFLite _radio;

void resetEEPROM();
void printEEPROM(int n = 0);
// void loadFromEEPROM();

void radioInit();
void connectToServer();
void listenForPackets();