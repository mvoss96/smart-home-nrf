#include <EEPROM.h>
#include <Arduino.h>
#include "config.h"
#include "blink.h"
#include "RFcomm.h"
#include "status.h"

void resetEEPROM()
{
    Serial.println(F("Reset EEPROM..."));
    blinkBoth(6, 200);
    for (size_t i = 0; i < EEPROM.length(); i++)
    {
        EEPROM.update(i, INITIAL_RADIO_ID);
    }
    serverConnected = false;
    radioID = INITIAL_RADIO_ID;
    serverUUID[0] = 0;
    serverUUID[1] = 0;
    serverUUID[2] = 0;
    serverUUID[3] = 0;
}

void printEEPROM(int n)
{
    if (n <= 0)
    {
        n = EEPROM.length();
    }
    for (int i = 0; i < n; i++)
    {
        uint8_t val = EEPROM.read(i);
        Serial.print(F("Address: "));
        Serial.print(i);
        Serial.print(F(" Value: "));
        if (val < 16)
            Serial.print('0');
        Serial.print(val, HEX);
        Serial.print(F(" ("));
        Serial.print(val);
        Serial.println(')');
    }
}

void loadFromEEPROM()
{
    // Check if Server was already set up and new ID saved
    Serial.println(F("Loading EEPROM..."));
    radioID = EEPROM.read(0);
    if (radioID != INITIAL_RADIO_ID)
    {
        serverUUID[0] = EEPROM.read(1);
        serverUUID[1] = EEPROM.read(2);
        serverUUID[2] = EEPROM.read(3);
        serverUUID[3] = EEPROM.read(4);
        // 5 and 6 reserved

        serverConnected = true;
        return;
    }
    else
    {
        Serial.println(F("EEPROM not setup"));
    }
    serverConnected = false;
}

void loadStatusFromEEPROM()
{
    unsigned statusOffset = 7;
    for (size_t i = 0; i < sizeof(status); i++)
    {
        ((uint8_t *)&status)[i] = EEPROM.read(statusOffset + i);
    }
}

void saveToEEPROM()
{
    Serial.println(F("Saving EEPROM..."));
    blinkBoth(2, 200);
    EEPROM.update(0, radioID);
    EEPROM.update(1, serverUUID[0]);
    EEPROM.update(2, serverUUID[1]);
    EEPROM.update(3, serverUUID[2]);
    EEPROM.update(4, serverUUID[3]);
    // 5 and 6 reserved
    unsigned statusOffset = 7;
    for (size_t i = 0; i < sizeof(status); i++)
    {
        EEPROM.update(statusOffset + i, ((uint8_t *)&status)[i]);
    }
}
