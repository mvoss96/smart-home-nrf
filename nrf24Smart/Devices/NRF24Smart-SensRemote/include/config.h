#pragma once
# include <Arduino.h>

#define IS_RF_REMOTE                                                                // Enable Remote RF fucntions

#define DEVICE_TYPE                                     "SensRemote"                // Max 20 characters
#define DEVICE_BATTERY_POWERED                          1                           // 0 for Stationary, 1 for Battery
#define DEVICE_BATTERY_FULL_VOLTAGE                     3000                        // In mV
#define DEVICE_BATTERY_EMPTY_VOLTAGE                    1800                        // In mV
#define VREFINT                                         1113                        // In mV must be calibrated
#define DEVICE_UUID                                     {0xD2, 0xF3, 0xB5, 0x12}    // Unique 4 byte UUID
#define LED_BLINK_ONMESSAGE                             0                           // 1 for enable, 0 for disable
#define STATUS_INTERVAL_TIME                            0                           // Not Supported
#define NUM_SEND_RETRIES                                2                           // Must be >= 1
#define FIRMWARE_VERSION                                0x01
#define RADIO_CHANNEL                                   101
#define INITIAL_RADIO_ID                                255
#define SERVER_RADIO_ID                                 0

#define PIN_RADIO_CE                                    9
#define PIN_RADIO_CSN                                   10
#define PIN_LED1                                        8
#define PIN_LED2                                        7
#define PIN_RST                                         A3
#define PIN_LISTEN                                      A0

#define PIN_BTN1_UP                                     6
#define PIN_BTN1_DN                                     5

#define SLEEP_AFTER_MS                                  750
#define DEBOUNCE_TIME_MS                                40
#define LONG_PRESS_INTERVAL_MS                          400
#define LONG_PRESS_MS                                   1000
#define DOUBLE_LONG_PRESS_MS                            2000
#define NUM_WAKEUP_FOR_STATUS                           37                           // Times 8s, 75 is 10 min




