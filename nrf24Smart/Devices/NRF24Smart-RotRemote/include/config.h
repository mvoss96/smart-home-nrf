#pragma once
# include <Arduino.h>

#define IS_RF_REMOTE                                                                // Enable Remote RF fucntions

#define DEVICE_TYPE                                     "RotRemote"                 // Max 32 characters long
#define DEVICE_BATTERY_POWERED                          1                           // 0 for Stationary, 1 for Battery
#define DEVICE_BATTERY_FULL_VOLTAGE                     3000                        // In mV
#define DEVICE_BATTERY_EMPTY_VOLTAGE                    1800                        // In mV
#define VREFINT                                         1113                        // In mV must be calibrated
#define DEVICE_UUID                                     {0xD2, 0xF1, 0xB2, 0x10}    // Unique 4 byte UUID
#define OUTPUT_POWER_LIMIT                              255                         // Maximum combined value of all channels
#define LED_BLINK_ONMESSAGE                             0                           // 1 for enable, 0 for disable
#define STATUS_INTERVAL_TIME                            0                           // Not Supported
#define NUM_SEND_RETRIES                                1                           // Must be >= 1

#define FIRMWARE_VERSION                                0x01
#define RADIO_CHANNEL                                   101
#define INITIAL_RADIO_ID                                255
#define SERVER_RADIO_ID                                 0
#define PIN_RADIO_CE                                    9
#define PIN_RADIO_CSN                                   10
#define PIN_LED1                                        A1
#define PIN_LED2                                        A0

#define PIN_ROTARY1                                     3
#define PIN_ROTARY2                                     2
#define PIN_BTN_ENC                                     4
#define PIN_BTN_RST                                     A4
#define PIN_BTN_LISTEN                                  5                             
#define DEBOUNCE_MS                                     40
#define LONG_PRESS_MS                                   500
#define DOUBLE_CLICK_MS                                 200
#define SLEEP_AFTER_MS                                  1500
#define CLICKS_PER_STEP                                 4

