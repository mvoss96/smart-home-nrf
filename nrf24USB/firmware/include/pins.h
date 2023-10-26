#pragma once
#define SERIAL_SPEED 115200
#define PIN_RADIO_CE 9
#define PIN_RADIO_CSN 10
#define PIN_RADIO_IRQ 2
#define PIN_LED1 A1
#define PIN_LED2 8

#define START_BYTE 0xFF
#define END_BYTE 0xFE
#define ESCAPE_BYTE 0xF0
#define DEVICE_NAME "NRF24USB"
#define TEXT_SEPERATOR ';'
#define BYTE_SEPERATOR ':'

const uint8_t FIRMWARE_VERSION = 2;
const uint8_t UUID[4] = {0x85, 0x83, 0xF7, 0x7E}; // 32bit UUID must be unique
enum class MSG_TYPES : uint8_t
{
    MSG,
    PAYLOAD,
    INIT,
    ERROR,
    OK,
    EMPTY,
    REBOOT,
    SETTING,
};

inline const char* MSG_TYPES_STR[] = {
    "MSG",
    "PAYLOAD",
    "INIT",
    "ERROR",
    "OK",
    "EMPTY",
    "REBOOT",
    "SETTING",
};

inline const char* msgTypeString(MSG_TYPES type) {
    return MSG_TYPES_STR[static_cast<uint8_t>(type)];
}


inline MSG_TYPES stringToMsgType(const char* str) {
    for (uint8_t i = 0; i < sizeof(MSG_TYPES_STR) / sizeof(MSG_TYPES_STR[0]); ++i) {
        if (strcmp(str, MSG_TYPES_STR[i]) == 0) {
            return static_cast<MSG_TYPES>(i);
        }
    }
    return MSG_TYPES::EMPTY;
}





