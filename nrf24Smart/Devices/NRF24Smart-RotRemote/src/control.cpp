#include "control.h"
#include "config.h"
#include "RFcomm.h"
#include "Arduino.h"
#include <NewEncoder.h>
#include <LowPower.h>
#include <CircularBuffer.h>
#include "power.h"
#include "blink.h"

enum class EVENTS : uint8_t
{
    CLICK,
    DOUBLE_CLICK,
    ROT_RIGHT,
    ROT_LEFT
};

CircularBuffer<EVENTS, 50> eventBuffer;

// Status object to keep track of various parameters
Status status;
uint8_t statusInterval = STATUS_INTERVAL_TIME;

NewEncoder encoder(2, 3, -20, 20, 0, FULL_PULSE);
int16_t prevEncoderValue;
long globalTimer;
bool wasRotated = false;
volatile bool sleeping = false;

void setStatus(const uint8_t *data, uint8_t length)
{
    // Null data check
    if (data == nullptr)
    {
        Serial.println("ERROR: Null data pointer in setStatus!");
        return;
    }

    SetMessage msg(data, length);
    if (!msg.isValid)
    {
        return;
    }

    // Check if newValue is not null before dereferencing
    if (msg.newValue == nullptr)
    {
        Serial.println("ERROR: Null newValue pointer!");
        return;
    }

    switch (msg.varIndex)
    {
    case 0: // TARGET
        if (msg.changeType == ChangeTypes::SET && msg.valueSize > 0)
        {
            if (msg.valueSize == 5)
            {
                status.targetID = msg.newValue[0];
                status.targetUUID[0] = msg.newValue[1];
                status.targetUUID[1] = msg.newValue[2];
                status.targetUUID[2] = msg.newValue[3];
                status.targetUUID[3] = msg.newValue[4];
                blink(PIN_LED2, 3, 100);
                saveToEEPROM();
            }
            else
            {
                Serial.println("ERROR: Incompatible valueSize for TARGET!");
            }
        }
        else
        {
            Serial.println("ERROR: Unsupported setType for TARGET!");
        }
        break;
    default:
    {
        Serial.println("ERROR: Unsupported changeType!");
    }
    }
}

void setupEncoder()
{
    NewEncoder::EncoderState state;
    if (!encoder.begin())
    {
        Serial.println("Encoder Failed to Start. Check pin assignments and available interrupts. Aborting.");
        while (1)
        {
            yield();
        }
    }
}

void rotClick()
{
    Serial.println("click ");
    globalTimer = millis();
    eventBuffer.push(EVENTS::CLICK);
    // sendRemote(LAYERS::BUTTONS, 0);
}

void rotDoubleClick()
{
    Serial.println("doubleclick ");
    globalTimer = millis();
    eventBuffer.push(EVENTS::DOUBLE_CLICK);
    // sendRemote(LAYERS::BUTTONS, 1);
}

void rotLeft()
{
    Serial.print("left ");
    wasRotated = true;
    globalTimer = millis();
    bool pressed = !digitalRead(PIN_BTN_ENC);
    Serial.println(pressed);
    eventBuffer.push(EVENTS::ROT_LEFT);
    // sendRemote((pressed) ? LAYERS::AXIS2 : LAYERS::AXIS1, (uint8_t)AXIS_DIRS::DOWN);
}

void rotRight()
{
    Serial.print("right ");
    wasRotated = true;
    globalTimer = millis();
    bool pressed = !digitalRead(PIN_BTN_ENC);
    Serial.println(pressed);
    eventBuffer.push(EVENTS::ROT_RIGHT);
    // sendRemote((pressed) ? LAYERS::AXIS2 : LAYERS::AXIS1, (uint8_t)AXIS_DIRS::UP);
}

void readButton()
{
    static bool lastPin = HIGH;
    static bool clicked = false;
    static unsigned long lastHigh = 0;
    static unsigned long lastLow = 0;
    static unsigned long lastClick = 0;

    if (clicked && wasRotated == false && millis() - lastClick >= DOUBLE_CLICK_MS)
    {
        clicked = false;
        rotClick();
    }

    if (digitalRead(PIN_BTN_ENC) == LOW && lastPin == HIGH)
    {
        if (millis() - lastHigh <= DEBOUNCE_MS)
        {
            return;
        }
        // Serial.println("pressed");
        clicked = false;
        globalTimer = millis();
        lastPin = LOW;
        lastLow = millis();
        wasRotated = false;
    }
    else if (digitalRead(PIN_BTN_ENC) == HIGH && lastPin == LOW)
    {
        if (millis() - lastLow < DEBOUNCE_MS)
        {
            return;
        }
        lastHigh = millis();
        lastPin = HIGH;
        if (millis() - lastLow > LONG_PRESS_MS && wasRotated == false)
        {
            Serial.println("Long press");
            return;
        }
        if (millis() - lastClick < DOUBLE_CLICK_MS)
        {
            rotDoubleClick();
            clicked = false;
        }
        else
        {
            // Serial.print("released ");
            // Serial.println(millis() - lastLow);
            clicked = true;
            globalTimer = millis();
            lastClick = millis();
        }
    }
}

void wakeup()
{
    if (sleeping)
    {

        Serial.println("woken up!");
        printPowerStatus();
        readButton();
        sendStatus();
        sleeping = false;
        globalTimer = millis();
    }
}

ISR(PCINT2_vect)
{
    // one of pins D0 to D7 has changed
}

void readEncoder()
{
    NewEncoder::EncoderState currentEncoderState;
    if (encoder.getState(currentEncoderState))
    {
        Serial.print("Encoder: ");
        globalTimer = millis();
        switch (currentEncoderState.currentClick)
        {
        case NewEncoder::UpClick:
            rotRight();
            break;

        case NewEncoder::DownClick:
            rotLeft();
            break;

        default:
            break;
        }
    }
    readButton();
}

void checkForSleep()
{
    if (millis() - globalTimer > SLEEP_AFTER_MS)
    {
        sleeping = true;
        Serial.println("sleep");
        delay(10);
        _radio.powerDown();
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

        Serial.println("woken up!");
        globalTimer = millis();
        readEncoder();
        sendStatus();
        sleeping = false;
        globalTimer = millis();
    }
}

void sendEvents()
{
    if (!eventBuffer.isEmpty())
    {
        EVENTS ev = eventBuffer.pop();
        switch (ev)
        {
        case EVENTS::CLICK:
            sendRemote(LAYERS::BUTTONS, 0);
            break;
        case EVENTS::DOUBLE_CLICK:
            sendRemote(LAYERS::BUTTONS, 1);
            break;
        case EVENTS::ROT_RIGHT:
            sendRemote(LAYERS::AXIS1, (uint8_t)AXIS_DIRS::UP);
            break;
        case EVENTS::ROT_LEFT:
            sendRemote(LAYERS::AXIS1, (uint8_t)AXIS_DIRS::DOWN);
            break;
        }
    }
}