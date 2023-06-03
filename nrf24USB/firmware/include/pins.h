#pragma once
#define SERIAL_SPEED 115200
#define PIN_RADIO_CE 9
#define PIN_RADIO_CSN 10
#define PIN_RADIO_IRQ 2
#define PIN_LED A1
#define PIN_EXT1 5
#define PIN_EXT2 6

#define START_BYTE 0xFF
#define END_BYTE 0xFE
#define ESCAPE_BYTE 0xF0

const uint8_t FIRMWARE_VERSION = 1;
const uint8_t UUID[4] = {0x85, 0x83, 0xF7, 0x7E}; // 32bit UUID must be unique
enum class MSG_TYPES : uint8_t
{
    MSG,
    PAYLOAD,
    INIT,
    ERROR,
    OK,
    INFO,
    REBOOT,
};
