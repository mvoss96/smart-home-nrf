#pragma once
#include <Arduino.h>

extern uint8_t statusInterval;

enum class ChangeTypes : uint8_t
{
    INVALID,
    SET,
    TOGGLE,
    INCREASE,
    DECREASE,
};

enum class LAYERS : uint8_t
{
    BUTTONS,
    AXIS1,
    AXIS2,
    AXIS3,
    AXIS4,
    AXIS5,
    AXIS6,
    AXIS7,
    AXIS8,
    AXIS9,
};

enum class AXIS_DIRS : uint8_t
{
    UP,
    DOWN,
};

enum class MSG_TYPES : uint8_t
{
    ERROR,
    INIT,
    BOOT,
    SET,
    RESET,
    STATUS,
    REMOTE,
    OK,
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
            Serial.println(F("ERROR: Too small SetMessage size!"));
            return;
        }
        varIndex = data[0];
        uint8_t rawChangeType = data[1];
        if (rawChangeType > static_cast<uint8_t>(ChangeTypes::DECREASE))
        {
            Serial.println(F("ERROR: Invalid changeType!"));
            return;
        }
        changeType = static_cast<ChangeTypes>(rawChangeType);
        valueSize = data[2];
        if (size - valueSize != 3)
        {
            Serial.println(F("ERROR: Incompatible SetMessage size!"));
            return;
        }
        newValue = data + 3;
        isValid = true;
    }
};