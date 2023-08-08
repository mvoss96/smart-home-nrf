#include "control.h"
#include "config.h"
#include "RFcomm.h"

// Status object to keep track of various parameters
Status status;
uint8_t statusInterval = STATUS_INTERVAL_TIME;


void setStatus(const uint8_t *data, uint8_t length)
{
}
