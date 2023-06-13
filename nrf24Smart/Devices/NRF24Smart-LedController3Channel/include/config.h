#pragma once

#define DEVICE_TYPE "LedController3Channel"
#define DEVICE_BATTERY_POWERED 0                // 0 for stationary, 1 for Battery
#define DEVICE_BATTERY_FULL_VOLTAGE 3000        // In mV
#define DEVICE_BATTERY_EMPTY_VOLTAGE 1800       // In mV
#define DEVICE_UUID { 0xB8, 0x66, 0xA2, 0xD4}   // Unique 4 byte UUID
#define FIRMWARE_VERSION 0x01
#define RADIO_CHANNEL 101
#define INITIAL_RADIO_ID 255
#define SERVER_RADIO_ID 0

#define PIN_RADIO_CE 9
#define PIN_RADIO_CSN 10
#define PIN_BTN1 A4
#define PIN_LED1 8
#define PIN_LED2 7
#define PIN_OUTPUT_R 6
#define PIN_OUTPUT_G 5
#define PIN_OUTPUT_B 3
#define VREFINT 1113                            // In mV must be calibrated