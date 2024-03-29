#pragma once
#include <Arduino.h>

#define SERIAL_SPEED 115200
#define PIN_RADIO_CE 9
#define PIN_RADIO_CSN 10
#define PIN_RADIO_IRQ 2
#define PIN_LED1 A1
#define PIN_LED2 8
#define USE_IRQ

#define DEVICE_NAME "NRF24USB"
#define MESSAGE_SEPERATOR ';'
#define ESCAPE_BYTE 0xF0
#define BYTE_SEPERATOR ':'
#define SERIAL_BUFFER_SIZE 128
#define INIT_MSG_PERIOD 5000

const uint8_t FIRMWARE_VERSION = 3;
const uint8_t UUID[4] = {0x85, 0x83, 0xF7, 0x7E}; // 32bit UUID must be unique

