#include "blink.h"
#include "config.h"

// Blink function takes  a count of blinks, and a duration for each blink
// It turns both LEDs on and off for the specified duration and count
void blinkBoth(uint8_t count, unsigned long duration)
{
  for (uint8_t i = 0; i < count; i++)
  {
    digitalWrite(PIN_LED1, LOW);
    digitalWrite(PIN_LED2, LOW);
    delay(duration);
    digitalWrite(PIN_LED1, HIGH);
    digitalWrite(PIN_LED2, HIGH);
    delay(duration);
  }
}

// Blink function takes a pin number, a count of blinks, and a duration for each blink
// It turns the LED on and off for the specified duration and count
void blink(uint8_t pin, uint8_t count, unsigned long duration)
{
  for (uint8_t i = 0; i < count; i++)
  {
    digitalWrite(pin, LOW);
    delay(duration);
    digitalWrite(pin, HIGH);
    delay(duration);
  }
}
