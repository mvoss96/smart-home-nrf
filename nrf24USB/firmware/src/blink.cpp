#include "blinkcodes.h"

// blink function takes a pin number, a count of blinks, and a duration for each blink
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

// blink_code function takes a blink_pattern (uint8_t) as input and
// calls the blink function with the appropriate parameters for each pattern
void blinkCode(uint8_t blink_pattern)
{
  switch (blink_pattern)
  {
  case BLINK_INIT_OK:
    blink(PIN_LED1, 1, 200);
    break;
  case BLINK_INIT_ERROR:
    blink(PIN_LED1, 1, 200);
    blink(PIN_LED1, 4, 100);
    break;
  case BLINK_ONCE:
    blink(PIN_LED1, 1, 50);
    break;
  default:
    break;
  }
}