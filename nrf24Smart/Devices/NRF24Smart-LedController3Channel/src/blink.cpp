#include "blink.h"

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
void blinkCode(uint8_t pin, BlinkCode blink_pattern)
{
  switch (blink_pattern)
  {
  case BlinkCode::BLINK_INIT_OK:
    blink(pin, 1, 200);
    break;
  case BlinkCode::BLINK_INIT_ERROR:
    blink(pin, 1, 200);
    blink(pin, 4, 100);
    break;
  case BlinkCode::BLINK_ONCE:
    blink(pin, 1, 50);
    break;
  default:
    break;
  }
}