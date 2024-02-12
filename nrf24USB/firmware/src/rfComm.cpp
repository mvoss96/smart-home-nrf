#include <NRFLite.h>
#include "serialComm.h"
#include "blinkcodes.h"

NRFLite _radio; // The nrflite radio instance
bool blinkOnMessage = true;
static volatile bool _dataWasReceived = false;
static volatile bool _SendFailed = false;
static volatile bool _SendSuccess = false;

void radioInterrupt()
{
    // Ask the radio what caused the interrupt.  This also resets the IRQ pin on the
    // radio so a new interrupt can be triggered.

    uint8_t txOk, txFail, rxReady;
    _radio.whatHappened(txOk, txFail, rxReady);

    // txOk = the radio successfully transmitted data.
    // txFail = the radio failed to transmit data.
    // rxReady = the radio received data.
    if (rxReady)
    {
        _dataWasReceived = true;
    }
    else if (txOk)
    {
        _SendSuccess = true;
    }
    else if (txFail)
    {
        _SendFailed = true;
    }
}

/**
 * @brief Initialize the radio module
 *
 * @param channel The radio channel to use
 * @param address The address of the radio module
 * @return true if the radio module was initialized successfully, false otherwise
 */
bool radioInit(uint8_t channel, uint8_t address)
{
    if (!_radio.init(address, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS, channel))
    {
        blinkCode(BLINK_INIT_ERROR);
        Serial.println("Radio Init Failed!");
        return false;
    }
#ifdef USE_IRQ
    attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);
#endif
    blinkCode(BLINK_INIT_OK);
    return true;
}

/**
 * @brief Listens for incoming data on the NRF24L01 radio module.
 *
 * @param buf Pointer to a buffer to store the received data
 * @param packetSize The size of the received packet as an output parameter
 * @return true if data is received, false otherwise.
 */
bool nrfListen(uint8_t *buf, uint8_t &packetSize)
{
#ifdef USE_IRQ
    if (!_dataWasReceived)
    {
        return false;
    }
    _dataWasReceived = false;
    packetSize = _radio.hasDataISR();
#else
    packetSize = _radio.hasData();
#endif
    if (packetSize > 0)
    {
        if (blinkOnMessage)
        {
            digitalWrite(PIN_LED2, LOW);
        }
        _radio.readData(buf);
        if (blinkOnMessage)
        {
            digitalWrite(PIN_LED2, HIGH);
        }
        return true;
    }
    return false;
}

/**
 * @brief Read the ack payload data and redirect it over the serial port to the host
 *
 * @return The lenght of the read ackPayload
 */
// uint8_t readAckPayload()
// {
//     uint8_t buff[32];
//     uint8_t length = _radio.hasAckData();
//     while (_radio.hasAckData())
//     {
//         _radio.readData(buff);
//     }

//     // Serial.print("length: ");
//     // Serial.print(length);
//     // Serial.print(" ");
//     // for(int i = 0; i< length; i++){
//     //     Serial.print((int)buff[i]);
//     //     Serial.print(" ");
//     // }
//     // Serial.println();
//     sendSerialPacket(buff, length, MSG_TYPES::OK);
//     return length;
// }

/**
 * @brief Send a message via the nrf radio
 *
 * @param destination The receiver ID
 * @param data A pointer to the message data
 * @param length The lenght of the data in bytes
 * @param requireAck Requiere acknolegement from receiver
 * @return true if the transmission succeeded else false
 */
bool nrfSend(uint8_t destination, void *data, uint8_t length, bool requireAck)
{
    if (blinkOnMessage)
    {
        digitalWrite(PIN_LED1, LOW);
    }

    // Serial.print("sending ");
    // for (int i = 0; i < length; i++)
    // {
    //     Serial.print(((uint8_t *)data)[i]);
    //     Serial.print(" ");
    // }
    // Serial.print("to address:");
    // Serial.println(destination);

    bool res = false;

#ifdef USE_IRQ
    _radio.startSend(destination, data, length, requireAck ? NRFLite::REQUIRE_ACK : NRFLite::NO_ACK);
    while (!_SendSuccess && !_SendFailed)
    {
        // Wait for send to complete
    }
    res = _SendSuccess;
    _radio.startRx();
#else
    res = _radio.send(destination, data, length, requireAck ? NRFLite::REQUIRE_ACK : NRFLite::NO_ACK);
#endif
    if (blinkOnMessage)
    {
        digitalWrite(PIN_LED1, HIGH);
    }
    return res;
}
