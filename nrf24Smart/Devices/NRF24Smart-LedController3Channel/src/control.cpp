#include "control.h"
#include "config.h"
#include "RFcomm.h"

// Status object to keep track of various parameters
Status status;

// Pointers to R, G, B channels in status object
uint8_t *channels[] = {&status.ch_1, &status.ch_2, &status.ch_3};

// Corresponding pins for R, G, B output
int pins[] = {PIN_OUTPUT_R, PIN_OUTPUT_G, PIN_OUTPUT_B};

// Config variables
unsigned int outputPowerLimit = OUTPUT_POWER_LIMIT;

// Function to update LED outputs based on the current status
void setOutput()
{
    int channelTotal = 0;

    // Calculate the channelTotal
    for (int i = 0; i < 3; i++)
    {
        channelTotal += *channels[i];
    }

    // Calculate the power scaling factor
    // Serial.print("total channel: ");
    // Serial.println(channelTotal);
    status.powerScale = min(1, (float)outputPowerLimit / channelTotal);
    // Serial.print("power scale: ");
    // Serial.println(status.powerScale);

    // Loop over each color channel
    for (int i = 0; i < 3; i++)
    {
        // If power is off, set output to LOW
        if (!status.power)
        {
            digitalWrite(pins[i], LOW);
        }
        // Else, set output according to brightness, color and power limit
        else
        {
            analogWrite(pins[i], *channels[i] * status.powerScale * (float)status.brightness / 255);
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
    status.brightness = max((uint8_t)(1 / status.powerScale), newBr);
}

// Function to increase brightness, taking care not to exceed 255
void increaseBrightness(uint8_t val)
{
    status.brightness = min(255, status.brightness + val);
}

// Function to decrease brightness, taking care not to go below 0
void decreaseBrightness(uint8_t val)
{
    status.brightness = max(1, status.brightness - val);
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

void setStatus(uint8_t *data, uint8_t length)
{
    SetMessage msg(data, length);
    if (!msg.isValid)
    {
        return;
    }
    switch (msg.varIndex)
    {
    case 0:
        if (msg.changeType == (uint8_t)ChangeTypes::SET)
        {
            setPower(*msg.newValue);
        }
        else if (msg.changeType == (uint8_t)ChangeTypes::TOGGLE)
        {
            togglePower();
        }
        else
        {
            Serial.println("ERROR: Unsupported setType for POWER!");
        }
        break;
    case 1:
        if (msg.changeType == (uint8_t)ChangeTypes::SET)
        {
            setBrightness(*msg.newValue);
        }
        else if (msg.changeType == (uint8_t)ChangeTypes::INCREASE)
        {
            increaseBrightness(*msg.newValue);
        }
        else if (msg.changeType == (uint8_t)ChangeTypes::DECREASE)
        {
            decreaseBrightness(*msg.newValue);
        }
        else
        {
            Serial.println("ERROR: Unsupported setType for BRIGHTNESS!");
        }
        break;
    case 2:
    case 3:
    case 4:
        if (msg.changeType == (uint8_t)ChangeTypes::SET)
        {
            setChannel(msg.varIndex - 2, *msg.newValue);
        }
        else if (msg.changeType == (uint8_t)ChangeTypes::INCREASE)
        {
            increaseChannel(msg.varIndex - 2, *msg.newValue);
        }
        else if (msg.changeType == (uint8_t)ChangeTypes::DECREASE)
        {
            decreaseChannel(msg.varIndex - 2, *msg.newValue);
        }
        else
        {
            Serial.print("ERROR: Unsupported setType for Channel ");
            Serial.println(msg.varIndex - 2);
        }
        break;
    case 5:
        if (msg.changeType == (uint8_t)ChangeTypes::SET)
        {
            if (msg.valueSize == 3)
            {
                setRGB(*(msg.newValue), *(msg.newValue + 1), *(msg.newValue + 2));
            }
            else
            {
                Serial.println("ERROR: Incompatible valueSize for RGB!");
            }
        }
        else
        {
            Serial.println("ERROR: Unsupported setType for RGB!");
        }
        break;
    case 6:
        if (msg.changeType == (uint8_t)ChangeTypes::SET)
        {
            if (msg.valueSize == 4)
            {
                outputPowerLimit = ((uint32_t)msg.newValue[0] << 24) | ((uint32_t)msg.newValue[1] << 16) |
                                   ((uint32_t)msg.newValue[2] << 8) | ((uint32_t)msg.newValue[3]);
            }
            else
            {
                Serial.println("ERROR: Incompatible valueSize for OutputPowerLimit!");
            }
        }
        else
        {
            Serial.println("ERROR: Unsupported setType for OutputPowerLimit!");
        }
        break;
    default:
    {
        Serial.println("ERROR: Unsupported changeType!");
    }
    }
    setOutput();
}
