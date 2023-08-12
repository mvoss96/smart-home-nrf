#pragma once

#define DEVICE_TYPE                                     "ExampleDevice"             // Max 20 characters long
#define DEVICE_BATTERY_POWERED                          0                           // 0 for Stationary, 1 for Battery
#define DEVICE_BATTERY_FULL_VOLTAGE                     3000                        // In mV
#define DEVICE_BATTERY_EMPTY_VOLTAGE                    1800                        // In mV
#define VREFINT                                         1113                        // In mV must be calibrated
#define DEVICE_UUID                                     {0x00, 0x01, 0x02, 0xF3}    // Unique 4 byte UUID
#define LED_BLINK_ONMESSAGE                             0                           // 1 for enable, 0 for disable
#define STATUS_INTERVAL_TIME                            5                           // in s
#define NUM_SEND_RETRIES                                1                           // Must be >= 1

#define FIRMWARE_VERSION                                0x01
#define RADIO_CHANNEL                                   101
#define INITIAL_RADIO_ID                                255
#define SERVER_RADIO_ID                                 0
#define PIN_RADIO_CE                                    9
#define PIN_RADIO_CSN                                   10
#define PIN_LED1                                        A1
#define PIN_LED2                                        A0

