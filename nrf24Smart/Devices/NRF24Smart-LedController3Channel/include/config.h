#pragma once

#define DEVICE_TYPE                                     "LedController3Channel"     // Max 32 characters long
#define NUM_LED_CHANNELS                                2                           // How many LED channels
#define DEVICE_BATTERY_POWERED                          0                           // 0 for stationary, 1 for Battery
#define DEVICE_BATTERY_FULL_VOLTAGE                     3000                        // In mV
#define DEVICE_BATTERY_EMPTY_VOLTAGE                    1800                        // In mV
#define VREFINT                                         1113                        // In mV must be calibrated
#define DEVICE_UUID                                     {0xB8, 0x66, 0xA2, 0xD6}    // Unique 4 byte UUID
#define OUTPUT_POWER_LIMIT                              255                         // Maximum combined value of all channels
#define LED_BLINK_ONMESSAGE                             1                           // 1 for enable, 0 for disable
#define STATUS_INTERVAL_TIME                            5                           // In seconds


#define FIRMWARE_VERSION                                0x01
#define RADIO_CHANNEL                                   101
#define INITIAL_RADIO_ID                                255
#define SERVER_RADIO_ID                                 0
#define PIN_RADIO_CE                                    9
#define PIN_RADIO_CSN                                   10
#define PIN_BTN1                                        A4
#define PIN_LED1                                        8
#define PIN_LED2                                        7
#define PIN_OUTPUT_R                                    6
#define PIN_OUTPUT_G                                    5
#define PIN_OUTPUT_B                                    3
