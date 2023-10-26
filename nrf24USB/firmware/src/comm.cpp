#include <Arduino.h>
#include <SPI.h>
#include <NRFLite.h>
#include "comm.h"
#include "blinkcodes.h"

NRFLite _radio; // The nrflite radio instance
const uint8_t BUFFER_SIZE = 128;
uint8_t buffer[BUFFER_SIZE];
bool clearTextMode = false;
bool blinkOnMessage = true;

/**
 * @brief Wait for Host Connection and initialize the radio
 *
 */
void waitForHost()
{
    long lastInitMsg = -10000;

    while (true)
    {
        if (millis() - lastInitMsg > 1000)
        {
            sendInitMessage();
            lastInitMsg = millis();
        }
        delay(100); // Short delay to let the host read the message
        int packetSize = readDataFromSerial(buffer, BUFFER_SIZE);
        if (packetSize >= 2) // Check if a correct packet was successfully received
        {
            MSG_TYPES receivedType = static_cast<MSG_TYPES>(buffer[0]); // Extract the packet type
            if (receivedType == MSG_TYPES::INIT)                        // Check if the packet type is INIT
            {
                uint8_t channel = buffer[1]; // Byte at position 1 is first data byte since byte 0 is msg_type
                uint8_t address = buffer[2];

                if (packetSize >= 3)
                {
                    clearTextMode = buffer[3];
                }
                if (packetSize >= 4)
                {
                    blinkOnMessage = buffer[4];
                }

                if (!testConnection(channel, address))
                {
                    for (;;)
                    {
                        sendStringMessage("NRF ERROR!", MSG_TYPES::ERROR);
                        delay(1000);
                    }
                }
                char msg[32];
                snprintf(msg, sizeof(msg), "NRF INIT channel:%d address:%d", channel, address);
                sendStringMessage(msg, MSG_TYPES::OK);
                break;
            }
        }
    }
}

void checkForSerialMsg()
{
    int packetSize = readDataFromSerial(buffer, BUFFER_SIZE);
    if (packetSize >= 0)
    {
        MSG_TYPES receivedType = static_cast<MSG_TYPES>(buffer[0]); // Extract the packet type
        uint8_t *data = buffer + 1;

        if (receivedType == MSG_TYPES::REBOOT)
        {
            sendStringMessage("REBOOT...", MSG_TYPES::REBOOT);
            delay(100);
            asm("JMP 0");
        }
        else if (receivedType == MSG_TYPES::SETTING && packetSize >= 1)
        {
            blinkOnMessage = data[0];
        }
        else if (receivedType == MSG_TYPES::MSG && packetSize >= 3)
        {
            uint8_t destinationID = data[0];
            uint8_t requireAck = data[1];
            nrfSend(destinationID, data + 2, packetSize - 2, static_cast<bool>(requireAck));
        }
    }
}

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
void sendPacketBS(uint8_t *data, uint8_t size, MSG_TYPES type)
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
 * @brief Sends an array of data bytes over the serial interface in clear text mode.
 *
 * @param data Pointer to the array of data bytes to be sent
 * @param size The number of data bytes to be sent from the array
 */
void sendPacketClear(uint8_t *data, uint8_t size, MSG_TYPES type)
{
    Serial.write(TEXT_SEPERATOR);
    Serial.write(DEVICE_NAME);
    Serial.write(BYTE_SEPERATOR);
    Serial.write(msgTypeString(type)); // Write the packet type
    Serial.write(BYTE_SEPERATOR);      // Write data
    for (uint8_t i = 0; i < size; i++)
    {
        uint8_t byteValue = data[i];
        char buffer[4];              // buffer to store the ASCII representation of the byte
        itoa(byteValue, buffer, 10); // Convert integer to string (base 10)

        // Write the numeric string
        for (char *p = buffer; *p != '\0'; ++p)
        {
            Serial.write(*p);
        }

        // Write separator, but avoid writing it after the last byte
        if (i < size - 1)
        {
            Serial.write(BYTE_SEPERATOR);
        }
    }
    Serial.write('\n');
    Serial.flush();
}

/**
 * @brief Sends an array of data bytes over the serial interface
 *
 * @param data Pointer to the array of data bytes to be sent
 * @param size The number of data bytes to be sent from the array
 */
void sendPacket(uint8_t *data, uint8_t size, MSG_TYPES type)
{
    if (clearTextMode)
    {
        sendPacketClear(data, size, type);
    }
    else
    {
        sendPacketBS(data, size, type);
    }
}

/**
 * @brief Send a string message
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

    int packetSize = _radio.hasData();
    if (packetSize > 0)
    {
        if (blinkOnMessage)
        {
            digitalWrite(PIN_LED2, LOW);
        }
        _radio.readData(&buf);
        sendPacket(buf, packetSize, MSG_TYPES::MSG);
        digitalWrite(PIN_LED2, HIGH);
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
    if (blinkOnMessage)
    {
        digitalWrite(PIN_LED1, LOW);
    }
    bool result = (_radio.send(destination, data, length, requireAck ? NRFLite::REQUIRE_ACK : NRFLite::NO_ACK) == 1);
    if (!result)
    {
        sendStringMessage("SEND ERROR", MSG_TYPES::ERROR);
    }
    else
    {
        readAckPayload();
    }
    digitalWrite(PIN_LED1, HIGH);
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
 * @return int The number of bytes received (excluding the message type), or -1 if no Packet
 */
int readDataFromSerialBS(uint8_t *buffer, uint8_t bufferSize)
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
            else
            {
                return -1; // Buffer Overflow
            }
        }
    }
    return -1; // No Complete Packet
}

/**
 * @brief Read data from the serial port using clear Text mode
 *
 * @param buffer An array to store the received data must be sufficiently big
 * @param bufferSize The size of the buffer array
 * @return int The number of bytes received (excluding the message type), or -1 if no Packet
 */
int readDataFromSerialClear(uint8_t *buffer, uint8_t bufferSize)
{
    static String incomingMessage = "";
    static bool messageStarted = false;
    while (Serial.available())
    {
        char incomingChar = Serial.read();

        // Start reading a new message
        if (incomingChar == ';')
        {
            messageStarted = true;
            incomingMessage = "";
        }
        // End of message
        else if (incomingChar == '\n' && messageStarted)
        {
            messageStarted = false;

            // Parsing message
            int received_bytes = 0;
            char *str = &incomingMessage[0];
            char *token = strtok(str, ":");
            if (token != NULL)
            {
                buffer[0] = (uint8_t)stringToMsgType(token); // Extract Message Type
                token = strtok(NULL, ":");
            }

            while (token != NULL)
            {
                if (received_bytes < bufferSize - 1)
                {
                    uint8_t value = (uint8_t)atoi(token);
                    buffer[1 + received_bytes++] = value;
                }
                else
                {
                    return -1; // Buffer Overflow
                }
                token = strtok(NULL, ":");
            }
            return received_bytes;
        }
        // Accumulate incoming bytes
        else if (messageStarted)
        {
            incomingMessage += incomingChar;
        }
    }
    return -1; // No Complete Packet
}

/**
 * @brief Read data from the serial port
 *
 * @param buffer An array to store the received data must be sufficiently big
 * @param bufferSize The size of the buffer array
 * @return int The number of bytes received (excluding the message type), or -1 if an error occurred
 */
int readDataFromSerial(uint8_t *buffer, uint8_t bufferSize)
{
    if (clearTextMode)
    {
        return readDataFromSerialClear(buffer, bufferSize);
    }
    else
    {
        return readDataFromSerialBS(buffer, bufferSize);
    }
}