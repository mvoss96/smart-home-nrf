#include "control.h"
#include "config.h"
#include "RFcomm.h"
#include <Wire.h>
#include <AHTxx.h>
#include <Arduino.h>
#include <LowPower.h>
#include "button.h"
#include <CircularBuffer.h>
#include "power.h"
#include "blink.h"

enum class EVENTS : uint8_t
{
    CLICK_UP,
    CLICK_DN,
    HOLD_UP,
    HOLD_DN,
};
CircularBuffer<EVENTS, 20> eventBuffer;
long globalTimer;

// Status object to keep track of various parameters
Status status;
uint8_t statusInterval = STATUS_INTERVAL_TIME;

volatile bool sleeping = false;
volatile unsigned wakeupcouter = 0;
volatile bool wakeupByInterrupt = false;
bool statusSend = false;

AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR); // sensor address, sensor type

Button buttonUp(PIN_BTN1_UP);
Button buttonDn(PIN_BTN1_DN);

ISR(PCINT2_vect)
{
    // Your code to execute when pins 5 or 6 change state
    wakeupByInterrupt = true;
    globalTimer = millis();
    sleeping = false;
}

// ISR for PORTC (covers A3 and A0)
ISR(PCINT1_vect)
{
    // Your ISR code for A3 and A0
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
                Serial.println(F("ERROR: Incompatible valueSize for TARGET!"));
            }
        }
        else
        {
            Serial.println(F("ERROR: Unsupported setType for TARGET!"));
        }
        break;
    default:
    {
        Serial.println(F("ERROR: Unsupported changeType!"));
    }
    }
}

void goToSleep()
{
    if (!sleeping)
    {
        sleeping = true;
        wakeupByInterrupt = false;
        Serial.println(F("sleep"));
        delay(10);
        _radio.powerDown();
    }
    Serial.flush();
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

void checkForSleep()
{
    if (millis() - globalTimer > SLEEP_AFTER_MS)
    {
        digitalWrite(PIN_LED2, HIGH);
        goToSleep();
        digitalWrite(PIN_LED2, LOW);

        if (!sleeping)
        {
            Serial.println(F("woken up!"));
            statusSend = false;
            readButtons();
            return;
        }
        wakeupcouter++;
        if (wakeupcouter >= NUM_WAKEUP_FOR_STATUS && !wakeupByInterrupt)
        {
            readSensor();
            sendStatus();
            wakeupcouter = 0;
        }
    }
}

void readButtons()
{
    // Prevent sleep while a button is pressed down
    if (digitalRead(PIN_BTN1_DN) == LOW || digitalRead(PIN_BTN1_UP) == LOW)
    {
        globalTimer = millis();
    }

    BTN_ACTIONS eventUp = buttonUp.read();
    BTN_ACTIONS eventDn = buttonDn.read();

    if (eventUp != BTN_NOTHING)
    {
        switch (eventUp)
        {
        case BTN_NOTHING:
            break;
        case BTN_CLICK:
            Serial.println("Click UP");
            eventBuffer.push(EVENTS::CLICK_UP);
            break;
        case BTN_LONG_PRESS_ACTIVE:
            Serial.println("Long Press UP");
            eventBuffer.push(EVENTS::HOLD_UP);
            break;
        }
    }
    else if (eventDn != BTN_NOTHING)
    {
        switch (eventDn)
        {
        case BTN_NOTHING:
            break;
        case BTN_CLICK:
            Serial.println("Click DN");
            eventBuffer.push(EVENTS::CLICK_DN);
            break;
        case BTN_LONG_PRESS_ACTIVE:
            Serial.println("Long Press DN");
            eventBuffer.push(EVENTS::HOLD_DN);
            break;
        }
    }
}

void readSensor()
{
    float ahtValue = aht20.readTemperature(); // read 6-bytes via I2C, takes 80 milliseconds
    if (ahtValue == AHTXX_ERROR)              // AHTXX_ERROR = 255, library returns 255 if error occurs
    {
        Serial.println("AHT-Sensor Error!");
        return;
    }
    status.temperature = ahtValue;
    status.humidity = aht20.readHumidity(AHTXX_USE_READ_DATA); // use 6-bytes from temperature reading, takes zero milliseconds
    delay(10);
    Serial.print("Temperature: ");
    Serial.print(status.temperature);
    Serial.print(" °C Humidity: ");
    Serial.print(status.humidity);
    Serial.println(" %");
}

void sendEvents()
{
    if (!eventBuffer.isEmpty())
    {
        while (!eventBuffer.isEmpty())
        {
            EVENTS ev = eventBuffer.pop();
            switch (ev)
            {
            case EVENTS::CLICK_UP:
                sendRemote(LAYERS::BUTTONS, 2);
                break;
            case EVENTS::CLICK_DN:
                sendRemote(LAYERS::BUTTONS, 3);
                break;
            case EVENTS::HOLD_UP:
                sendRemote(LAYERS::AXIS1, (uint8_t)AXIS_DIRS::UP);
                break;
            case EVENTS::HOLD_DN:
                sendRemote(LAYERS::AXIS1, (uint8_t)AXIS_DIRS::DOWN);
                break;
            }
        }
        // readSensor();
        if (!statusSend)
        {
            sendStatus();
            statusSend = true;
        }
    }
}
