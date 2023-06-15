#pragma once
#include <Arduino.h>

struct Status{
    uint8_t power = true;
    uint8_t brightness = 127;
    uint8_t ch_1 = 255;
    uint8_t ch_2 = 255;
    uint8_t ch_3 = 255;
};

extern Status status;

void setOutput();
void togglePower();
void setPower(bool newPwr);
void setBrightness(uint8_t newBr);
void increaseBrightness(uint8_t val);
void decreaseBrightness(uint8_t val);
void setChannel(int channel, uint8_t value);
void increaseChannel(int channel, uint8_t val);
void decreaseChannel(int channel, uint8_t val);
void setRGB(uint8_t r, uint8_t g, uint8_t b);


