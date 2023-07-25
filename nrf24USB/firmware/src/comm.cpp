#include <Arduino.h>
#include <SPI.h>
#include <NRFLite.h>
#include "comm.h"
#include "blinkcodes.h"

NRFLite _radio; // The nrflite radio instance

/**
 * @brief Test the connection and initialisation to the nrf24 radio
 *
 * @return true if  successfull
 * @return false if uncussefull
 */
bool testConnection(uint8_t channel, uint8_t address)
{
    if (!_radio.init(address, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS, channel))
    {
        blinkCode(BLINK_INIT_ERROR);
        // Serial.println("nrf error");
        return false;
    }
    blinkCode(BLINK_INIT_OK);
    return true;
}

/**
 * @brief Check if a byte is a special byte (start, end, escape, or status message)
 *
 * @param byte The byte to check
 * @return true If the byte is a special byte
 * @return false If the byte is not a special byte
 */
bool isSpecialByte(uint8_t byte)
{
    return (byte == START_BYTE || byte == END_BYTE || byte == ESCAPE_BYTE);
}

/**
 * @brief Sends an array of data bytes over the serial interface with byte stuffing.
 *        The function adds a start byte, packet size, and end byte to the data stream.
 *        Special bytes (start, end, and escape) within the data are escaped to avoid confusion.
 *
 * @param data Pointer to the array of data bytes to be sent
 * @param size The number of data bytes to be sent from the array
 */
void sendPacket(uint8_t *data, uint8_t size, MSG_TYPES type)
{
    Serial.write(START_BYTE);                 // Write the start byte
    Serial.write(static_cast<uint8_t>(type)); // Write the packet type
    for (uint8_t i = 0; i < size; i++)
    {
        // If the data byte is a special byte (start, end, or escape), send an escape byte followed by the data byte
        if (isSpecialByte(data[i]))
        {
            Serial.write(ESCAPE_BYTE);
            Serial.write(data[i]);
        }
        else
        {
            Serial.write(data[i]);
        }
    }
    Serial.write(END_BYTE); // Write the end byte
}

/**
 * @brief Send a string message using the custom protocol with byte stuffing
 *
 * @param message A null-terminated C-string containing the status message
 */
void sendStringMessage(const char *message, MSG_TYPES type)
{
    sendPacket((uint8_t *)message, strlen(message), type);
}

/**
 * @brief Send the init message containig the firmware version and the uuid
 *
 */
void sendInitMessage()
{
    uint8_t message[5] = {FIRMWARE_VERSION, UUID[0], UUID[1], UUID[2], UUID[3]};
    sendPacket((uint8_t *)message, 5, MSG_TYPES::INIT);
}

/**
 * @brief Listen for nrf messages and forward them onto the serial port
 *
 */
void nrfListen()
{
    // Init buffer to 0;
    uint8_t buf[32] = {0};

    int packetSize =  _radio.hasData();
    if (packetSize > 0)
    {
        _radio.readData(&buf);
        sendPacket(buf, packetSize, MSG_TYPES::MSG);
    }
}

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
    //digitalWrite(PIN_LED, LOW);
    bool result = (_radio.send(destination, data, length, requireAck ? NRFLite::REQUIRE_ACK : NRFLite::NO_ACK) == 1);
    if (!result)
    {
        sendStringMessage("SEND ERROR", MSG_TYPES::ERROR);
    }
    else
    {
        readAckPayload();
    }
    //digitalWrite(PIN_LED, HIGH);
    return result;
}

/**
 * @brief Read the ack payload data and redirect it over the serial port to the host
 *
 * @return The lenght of the read ackPayload
 */
uint8_t readAckPayload()
{
    uint8_t buff[32];
    uint8_t length = _radio.hasAckData();
    while (_radio.hasAckData())
    {
        _radio.readData(buff);
    }

    // Serial.print("length: ");
    // Serial.print(length);
    // Serial.print(" ");
    // for(int i = 0; i< length; i++){
    //     Serial.print((int)buff[i]);
    //     Serial.print(" ");
    // }
    // Serial.println();
    sendPacket(buff, length, MSG_TYPES::OK);
    return length;
}

/**
 * @brief Read data from the serial port using the custom protocol with byte stuffing
 *
 * @param buffer An array to store the received data must be sufficiently big
 * @param bufferSize The size of the buffer array
 * @return int The number of bytes received (excluding the message type), or -1 if an error occurred
 */
int readDataFromSerial(uint8_t *buffer, uint8_t bufferSize)
{
    static uint8_t received_bytes = 0;
    static bool in_escape = false;
    while (Serial.available())
    {
        uint8_t byte = Serial.read();

        if (!in_escape && byte == ESCAPE_BYTE)
        {
            in_escape = true;
        }
        else
        {
            if (!in_escape)
            {
                if (byte == START_BYTE)
                {
                    received_bytes = 0;
                    continue;
                }
                if (byte == END_BYTE)
                {
                    return received_bytes - 1; // Subtract 1 for the message type
                }
            }
            else
            {
                in_escape = false;
            }
            if (received_bytes < bufferSize)
            {
                buffer[received_bytes++] = byte;
            }
        }
    }
    return -1; // Incomplete packet or timeout
}