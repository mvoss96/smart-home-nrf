#pragma once
#include <Arduino.h>
#include "config.h"

void setStatus(const uint8_t *data, uint8_t length);
void setRemote(const uint8_t layer, const uint8_t value);

// Device Settings struct as it is stored in EEPROM
struct DeviceSettings
{
    uint8_t UUID[4] = DEVICE_UUID;
    bool allowRemote = ALLOW_REMOTE;
    bool ledBlinkOnMessage = LED_BLINK_ONMESSAGE;
    uint8_t minBrightness = MIN_BRIGHTNESS;
    uint8_t controlSteps = CONTROL_STEPS;
    uint8_t numLedChannels = NUM_LED_CHANNELS;
    uint16_t outputPowerLimit = OUTPUT_POWER_LIMIT;

    void print()
    {
        Serial.print(F("Device Settings Struct: UUID: "));
        for (int i = 0; i < 4; i++)
        {
            Serial.print(UUID[i], HEX);
            Serial.print(F(", "));
        }
        Serial.print(F("Allow Remote: "));
        Serial.print(allowRemote);
        Serial.print(F(", LED Blink on Message: "));
        Serial.print(ledBlinkOnMessage);
        Serial.print(F(", Minimum Brightness: "));
        Serial.print(minBrightness);
        Serial.print(F(", Control Steps: "));
        Serial.print(controlSteps);
        Serial.print(F(", Number of LED Channels: "));
        Serial.print(numLedChannels);
        Serial.print(F(", Output Power Limit: "));
        Serial.println(outputPowerLimit);
    }
};
extern DeviceSettings deviceSettings;

// Status struct as it is stored in EEPROM or sent over the air
struct Status
{
    uint8_t power = true;
    uint8_t brightness = 127;
    uint8_t ch_1 = 255;
    uint8_t ch_2 = (deviceSettings.numLedChannels >= 2) ? 255 : 0;
    uint8_t ch_3 = (deviceSettings.numLedChannels >= 3) ? 255 : 0;
    uint8_t num_channels = deviceSettings.numLedChannels;
    float powerScale = 1.0;

    void print()
    {
        Serial.print(F("Status Struct: Power: "));
        Serial.print(power);
        Serial.print(F(", Brightness: "));
        Serial.print(brightness);
        Serial.print(F(", Channel 1: "));
        Serial.print(ch_1);
        Serial.print(F(", Channel 2: "));
        Serial.print(ch_2);
        Serial.print(F(", Channel 3: "));
        Serial.print(ch_3);
        Serial.print(F(", Number of Channels: "));
        Serial.print(num_channels);
        Serial.print(F(", Power Scale: "));
        Serial.println(powerScale);
    }
};
extern Status status;


