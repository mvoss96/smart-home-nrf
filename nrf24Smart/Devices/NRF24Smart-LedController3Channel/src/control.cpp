#include "control.h"
#include "config.h"

// Status object to keep track of various parameters
Status status;

// Pointers to R, G, B channels in status object
uint8_t *channels[] = {&status.ch_1, &status.ch_2, &status.ch_3};

// Corresponding pins for R, G, B output
int pins[] = {PIN_OUTPUT_R, PIN_OUTPUT_G, PIN_OUTPUT_B};

// Function to update LED outputs based on the current status
void setOutput()
{
    // Loop over each color channel
    for (int i = 0; i < 3; i++)
    {
        // If power is off, set output to LOW
        if (!status.power)
        {
            digitalWrite(pins[i], LOW);
        }
        // Else, set output according to brightness and color
        else
        {
            analogWrite(pins[i], *channels[i] * status.brightness / 255);
        }
    }
}

// Function to toggle power status
void togglePower()
{
    status.power = !status.power;
}

// Function to set power status
void setPower(bool newPwr)
{
    status.power = newPwr;
}

// Function to set brightness
void setBrightness(uint8_t newBr)
{
    status.brightness = newBr;
}

// Function to increase brightness, taking care not to exceed 255
void increaseBrightness(uint8_t val)
{
    status.brightness = min(255, status.brightness + val);
}

// Function to decrease brightness, taking care not to go below 0
void decreaseBrightness(uint8_t val)
{
    status.brightness = max(0, status.brightness - val);
}

// Function to set a specific color channel to a given value
void setChannel(int channel, uint8_t value)
{
    *channels[channel] = value;
}

// Function to increase a specific color channel, taking care not to exceed 255
void increaseChannel(int channel, uint8_t val)
{
    *channels[channel] = min(255, *channels[channel] + val);
}

// Function to decrease a specific color channel, taking care not to go below 0
void decreaseChannel(int channel, uint8_t val)
{
    *channels[channel] = max(0, *channels[channel] - val);
}

// Function to set R, G, B values at once
void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    *channels[0] = r;
    *channels[1] = g;
    *channels[2] = b;
}
