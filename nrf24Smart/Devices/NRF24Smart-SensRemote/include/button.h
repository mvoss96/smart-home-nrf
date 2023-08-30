#pragma once
#include <Arduino.h>
#include "config.h"

enum BTN_ACTIONS : uint8_t
{
    BTN_NOTHING,
    BTN_CLICK,
    BTN_LONG_PRESS_ACTIVE,
};

class Button
{
private:
    int pin;
    bool lastState;
    bool longPressStart;
    unsigned long lastLow;
    unsigned long lastLongPressActive;

public:
    Button(int p) : pin(p), lastState(HIGH), longPressStart(false), lastLow(0), lastLongPressActive(0) {}

    BTN_ACTIONS read()
    {
        BTN_ACTIONS event = BTN_NOTHING;
        bool currentState = digitalRead(pin);

        if (currentState == LOW && lastState == HIGH)
        {
            lastLow = millis();
        }
        // Click
        else if (currentState == HIGH && lastState == LOW)
        {
            unsigned long lastHigh = millis();
            if (lastHigh - lastLow > DEBOUNCE_TIME_MS)
            {
                if (lastHigh - lastLow < LONG_PRESS_INTERVAL_MS)
                {
                    event = BTN_CLICK;
                }
                longPressStart = false;
            }
        }
        // Long Press
        else if (currentState == LOW && lastState == LOW && millis() - lastLongPressActive > LONG_PRESS_INTERVAL_MS)
        {
            if (longPressStart)
            {
                event = BTN_LONG_PRESS_ACTIVE;
            }
            else
            {
                longPressStart = true;
            }
            lastLongPressActive = millis();
        }

        lastState = currentState;
        return event;
    }
};
