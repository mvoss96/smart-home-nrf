#pragma once
#include "pins.h"

enum class MSG_TYPES : uint8_t
{
    NONE,
    OK,
    ERROR,
    MSG,
    INIT,
    SETTING,
    REBOOT,
};

inline const char *MSG_TYPES_STR[] = {
    "NONE",
    "OK",
    "ERROR",
    "MSG",
    "INIT",
    "SETTING",
    "REBOOT",
};

void waitForHost();
void checkForSerialMsg();
bool testConnection(uint8_t channel, uint8_t address);
void sendStringMessage(const char *message, MSG_TYPES type);
void sendInitMessage();
void nrfListen();
bool nrfSend(uint8_t destination, void *data, uint8_t length, bool requireAck = true);
uint8_t readAckPayload();

inline const char *msgTypeString(MSG_TYPES type)
{
    size_t arraySize = sizeof(MSG_TYPES_STR) / sizeof(MSG_TYPES_STR[0]);
    if (static_cast<size_t>(type) < arraySize)
    {
        return MSG_TYPES_STR[static_cast<uint8_t>(type)];
    }
    return "UNKNOWN"; // or some other appropriate default value
}

inline MSG_TYPES stringToMsgType(const char *str)
{
    for (uint8_t i = 0; i < sizeof(MSG_TYPES_STR) / sizeof(MSG_TYPES_STR[0]); ++i)
    {
        if (strcmp(str, MSG_TYPES_STR[i]) == 0)
        {
            return static_cast<MSG_TYPES>(i);
        }
    }
    return MSG_TYPES::NONE;
}

class SerialMessage
{
private:
    bool valid = false;
    MSG_TYPES type = MSG_TYPES::NONE;
    uint8_t data[SERIAL_BUFFER_SIZE] = {0};
    uint8_t dataSize = 0;

public:
    SerialMessage(const uint8_t *buffer, const uint8_t len)
    {
        if (len == 0) // Check for empty buffer
        {
            Serial.println("SerialMessage: empty buffer!");
            return;
        }
        
        //Serial.print("SerialMessage:");
        //Serial.print(len);
        //Serial.print(" ");
        //Serial.println((char*)buffer);

        // Try finding a BYTE_SEPERATOR in the buffer to determine if it's a string-form message
        char *separator = strchr((char *)buffer, BYTE_SEPERATOR);
        if (separator != nullptr)
        {
            //Serial.print("Seperator:");
            //Serial.println(*separator);
            *separator = '\0';
            type = stringToMsgType((char *)buffer);

            //Serial.print("serial type:");
            //Serial.println((uint8_t)type);

            if (type != MSG_TYPES::NONE)
            {
                // Tokenize the string and convert to bytes
                char separatorStr[] = {BYTE_SEPERATOR, '\0'}; // Create a temporary string for strtok
                char *token = strtok(separator + 1, separatorStr);

                while (token != nullptr)
                {
                    // Check if the token is a valid number
                    for (char *ptr = token; *ptr != '\0'; ++ptr)
                    {
                        if (!isdigit(*ptr))
                        {
                            Serial.print("SerialMessage: invalid char in token: ");
                            Serial.println(*ptr);
                            return; // Invalid character in token}
                        }
                    }

                    int value = atoi(token);
                    // Check if the value can be stored as a uint8_t
                    if (value < 0 || value > 255)
                    {
                        Serial.print("SerialMessage: invalid value: ");
                        Serial.println(value);
                        return; // Invalid byte value
                    }

                    data[dataSize++] = (uint8_t)value;
                    token = strtok(nullptr, separatorStr);
                }
                valid = true; // valid text message
                return;
            }
        }

        // Fall through to byte-form message processing for unrecognized or non-string messages
        type = (MSG_TYPES)buffer[0];
        dataSize = len - 1;
        memmove(data, buffer + 1, dataSize); // Shift buffer 1 to the left to remove msgType
        valid = true;
    }

    MSG_TYPES getType() { return type; }
    uint8_t getdataSize() { return dataSize; }
    const uint8_t *getData() { return data; }
    const bool isValid() { return valid; }
    void print()
    {
        Serial.print("type:");
        Serial.print((uint8_t)type);
        Serial.print(" ");
        Serial.print(msgTypeString(type));
        Serial.print(" ");
        for (int i = 0; i < dataSize; i++)
        {
            Serial.print(data[i]);
            if (i < dataSize - 1)
            {
                Serial.print("-");
            }
        }
        Serial.println();
    }
};