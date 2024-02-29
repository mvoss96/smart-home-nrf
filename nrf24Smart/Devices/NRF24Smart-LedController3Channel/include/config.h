#pragma once
#include "RFconfig.h"
#include "CONFIG_HARDWARE.h"

#define FIRMWARE_VERSION                                0x02
#define DEVICE_TYPE                                     DEVICE_LED_CONTROLLER_3CH
#define NUM_SEND_RETRIES                                1                           // Must be >= 1


// Device Settings will only be written to EEPROM if they are not already present or if forced
//#define FORCE_DEVICE_SETTINGS_REWRITE                                                    // Force Settings Rewrite
#define DEVICE_UUID                                     {0xB8, 0x66, 0xA2, 0xD7}    // Unique 4 byte UUID
#define ALLOW_REMOTE                                    1                           // Allow Remote Control (1 for enable, 0 for disable)
#define LED_BLINK_ONMESSAGE                             0                           // 1 for enable, 0 for disable
#define MIN_BRIGHTNESS                                  8                           // Minimum brightness value (0-255)
#define CONTROL_STEPS                                   16                          // Size of the control steps
#define NUM_LED_CHANNELS                                1                           // How many LED channels
#define OUTPUT_POWER_LIMIT                              255*NUM_LED_CHANNELS        // Maximum combined value of all channels







