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
uint8_t statusInterval = STATUS_INTERVAL_TIME;

void disableOutput()
{
    for (auto pin : pins)
    {
        digitalWrite(pin, LOW);
    }
}

// Function to update LED outputs based on the current status
void setOutput()
{
    int channelTotal = 0;

    // Calculate the channelTotal
    for (int i = 0; i < NUM_LED_CHANNELS; i++)
    {
        channelTotal += *channels[i];
    }

    if (channelTotal == 0)
    {
        Serial.println(F("channelTotal is 0!"));
        disableOutput();
        return;
    }

    // Calculate the power scaling factor
    // Serial.print("total channel: ");
    // Serial.println(channelTotal);
    status.powerScale = min(1, (float)outputPowerLimit / channelTotal);
    // Serial.print("power scale: ");
    // Serial.println(status.powerScale);

    // Loop over each color channel
    for (int i = 0; i < NUM_LED_CHANNELS; i++)
    {
        // If power is off, set output to LOW
        if (!status.power)
        {
            disableOutput();
        }
        // Else, set output according to brightness, color and power limit
        else if (channels[i]) // make sure pointer isn't NULL before dereferencing
        {
            uint8_t value = *channels[i] * status.powerScale * (float)status.brightness / 255;
            analogWrite(pins[i], value);
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
    if (status.powerScale != 0) // Prevent divide by zero error
    {
        status.brightness = max((uint8_t)(1 / status.powerScale), newBr);
    }
}

// Function to increase brightness, taking care not to exceed 255
void increaseBrightness(uint8_t val)
{
    status.brightness = min(255, status.brightness + val);
}

// Function to decrease brightness, taking care not to go below 0
void decreaseBrightness(uint8_t val)
{
    status.brightness = max(MIN_BRIGHTNESS, status.brightness - val);
}

// Function to set a specific color channel to a given value
void setChannel(int channel, uint8_t value)
{
    if (channels[channel]) // Make sure pointer isn't NULL before dereferencing
    {
        *channels[channel] = value;
    }
}

// Function to increase a specific color channel, taking care not to exceed 255
void increaseChannel(int channel, uint8_t val)
{
    if (channels[channel]) // make sure pointer isn't NULL before dereferencing
    {
        *channels[channel] = min(255, *channels[channel] + val);
    }
}

// Function to decrease a specific color channel, taking care not to go below 0
void decreaseChannel(int channel, uint8_t val)
{
    if (channels[channel]) // make sure pointer isn't NULL before dereferencing
    {
        *channels[channel] = max(0, *channels[channel] - val);
    }
}

// Function to set R, G, B values at once
void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    if (channels[0]) // Make sure pointer isn't NULL before dereferencing
    {
        *channels[0] = r;
    }
    if (channels[1]) // Make sure pointer isn't NULL before dereferencing
    {
        *channels[1] = g;
    }
    if (channels[2]) // Make sure pointer isn't NULL before dereferencing
    {
        *channels[2] = b;
    }
}

void setStatus(const uint8_t *data, uint8_t length)
{
    // Null data check
    if (data == nullptr)
    {
        Serial.println(F("ERROR: Null data pointer in setStatus!"));
        return;
    }

    SetMessage msg(data, length);
    if (!msg.isValid)
    {
        Serial.println(F("Set Message not valid!"));
        return;
    }

    // Check if newValue is not null before dereferencing
    if (msg.newValue == nullptr)
    {
        Serial.println(F("ERROR: Null newValue pointer!"));
        return;
    }

    switch (msg.varIndex)
    {
    case 0: // POWER
        if (msg.changeType == ChangeTypes::SET && msg.valueSize > 0)
        {
            setPower(*msg.newValue);
        }
        else if (msg.changeType == ChangeTypes::TOGGLE)
        {
            togglePower();
        }
        else
        {
            Serial.println(F("ERROR: Unsupported setType for POWER!"));
        }
        break;
    case 1: // BRIGHTNESS
        if (msg.changeType == ChangeTypes::SET && msg.valueSize > 0)
        {
            setBrightness(*msg.newValue);
        }
        else if (msg.changeType == ChangeTypes::INCREASE && msg.valueSize > 0)
        {
            increaseBrightness(*msg.newValue);
        }
        else if (msg.changeType == ChangeTypes::DECREASE && msg.valueSize > 0)
        {
            decreaseBrightness(*msg.newValue);
        }
        else
        {
            Serial.println(F("ERROR: Unsupported setType for BRIGHTNESS!"));
        }
        break;
    case 2: // CH_1
    case 3: // CH_2
    case 4: // CH_3
        if (msg.changeType == ChangeTypes::SET && msg.valueSize > 0)
        {
            setChannel(msg.varIndex - 2, *msg.newValue);
        }
        else if (msg.changeType == ChangeTypes::INCREASE && msg.valueSize > 0)
        {
            increaseChannel(msg.varIndex - 2, *msg.newValue);
        }
        else if (msg.changeType == ChangeTypes::DECREASE && msg.valueSize > 0)
        {
            decreaseChannel(msg.varIndex - 2, *msg.newValue);
        }
        else
        {
            Serial.print(F("ERROR: Unsupported setType for Channel "));
            Serial.println(msg.varIndex - 2);
        }
        break;
    case 5: // RGB
        if (msg.changeType == ChangeTypes::SET && msg.valueSize > 0)
        {
            if (msg.valueSize == 3)
            {
                setRGB(*(msg.newValue), *(msg.newValue + 1), *(msg.newValue + 2));
            }
            else
            {
                Serial.println(F("ERROR: Incompatible valueSize for RGB!"));
            }
        }
        else
        {
            Serial.println(F("ERROR: Unsupported setType for RGB!"));
        }
        break;
    case 6: // OUTPUT_POWER
        if (msg.changeType == ChangeTypes::SET)
        {
            if (msg.valueSize == 4)
            {
                outputPowerLimit = ((uint32_t)msg.newValue[0] << 24) | ((uint32_t)msg.newValue[1] << 16) |
                                   ((uint32_t)msg.newValue[2] << 8) | ((uint32_t)msg.newValue[3]);
            }
            else
            {
                Serial.println(F("ERROR: Incompatible valueSize for OutputPowerLimit!"));
            }
        }
        else
        {
            Serial.println(F("ERROR: Unsupported setType for OutputPowerLimit!"));
        }
        break;
    case 7: // STATUS_INTERVAL
        if (msg.changeType == ChangeTypes::SET && msg.valueSize > 0)
        {
            statusInterval = max(1, *msg.newValue);
        }
        else
        {
            Serial.println(F("ERROR: Unsupported setType for StatusInterval!"));
        }
        break;
    default:
    {
        Serial.println(F("ERROR: Unsupported changeType!"));
    }
    }
    setOutput();
}

void setRemote(const uint8_t layer, const uint8_t value)
{
    if (layer == (uint8_t)LAYERS::BUTTONS)
    {
        switch (value)
        {
        case 0:
            setPower(!status.power);
            break;
        case 1:
            for (int i = 0; i < NUM_LED_CHANNELS; i++)
            {
                setChannel(i, 255);
            }
            break;
        case 2:
            setPower(true);
            break;
        case 3:
            setPower(false);
            break;
        default:
            Serial.print(F("Unsupported Remote Buton: "));
            Serial.println(value);
        }
    }
    else if (layer == (uint8_t)LAYERS::AXIS1)
    {
        if (value == (uint8_t)AXIS_DIRS::UP)
        {
            increaseBrightness(CONTROL_STEPS);
        }
        else if (value == (uint8_t)AXIS_DIRS::DOWN)
        {
            decreaseBrightness(CONTROL_STEPS);
        }
        else
        {
            Serial.print(F("Unsupported AXIS1 Direction: "));
            Serial.println(value);
        }
    }
    else if (layer == (uint8_t)LAYERS::AXIS2 && NUM_LED_CHANNELS == 2)
    {
        if (value == (uint8_t)AXIS_DIRS::UP)
        {
            increaseChannel(0, CONTROL_STEPS);
            decreaseChannel(1, CONTROL_STEPS);
        }
        else if (value == (uint8_t)AXIS_DIRS::DOWN)
        {
            increaseChannel(1, CONTROL_STEPS);
            decreaseChannel(0, CONTROL_STEPS);
        }
        else
        {
            Serial.print(F("Unsupported AXIS2 Direction: "));
            Serial.println(value);
        }
    }
    setOutput();
}