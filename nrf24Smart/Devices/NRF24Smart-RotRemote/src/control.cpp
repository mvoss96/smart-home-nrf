#include "control.h"
#include "config.h"
#include "RFcomm.h"
#include "Arduino.h"
#include <NewEncoder.h>
#include <LowPower.h>
#include "power.h"

// Status object to keep track of various parameters
Status status;
uint8_t statusInterval = STATUS_INTERVAL_TIME;

NewEncoder encoder(2, 3, -20, 20, 0, FULL_PULSE);
int16_t prevEncoderValue;
long globalTimer;
bool wasRotated = false;

void setStatus(const uint8_t *data, uint8_t length)
{
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
    sendRemote(LAYERS::BUTTONS, 0);
}

void rotDoubleClick()
{
    Serial.println("doubleclick ");
    globalTimer = millis();
    sendRemote(LAYERS::BUTTONS, 1);
}

void rotLeft()
{
    Serial.print("left ");
    wasRotated = true;
    globalTimer = millis();
    bool pressed = !digitalRead(PIN_BTN_ENC);
    Serial.println(pressed);
    sendRemote((pressed) ? LAYERS::AXIS2 : LAYERS::AXIS1, (uint8_t)AXIS_DIRS::DOWN);
}

void rotRight()
{
    Serial.print("right ");
    wasRotated = true;
    globalTimer = millis();
    bool pressed = !digitalRead(PIN_BTN_ENC);
    Serial.println(pressed);
    sendRemote((pressed) ? LAYERS::AXIS2 : LAYERS::AXIS1, (uint8_t)AXIS_DIRS::UP);
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

ISR(PCINT2_vect)
{
    // one of pins D0 to D7 has changed
    globalTimer = millis();
    readEncoder();
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
    static bool sleeping = false;
    if (sleeping)
    {
        Serial.println("woken up!");
        sleeping = false;
    }
    if (millis() - globalTimer > SLEEP_AFTER_MS)
    {
        sleeping = true;
        delay(500); // wait for voltage to stabilize
        printPowerStatus();
        sendStatus();
        Serial.println("sleep");
        Serial.println("Disable Radio");
        _radio.powerDown();
        delay(100);
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        globalTimer = millis();
    }
}