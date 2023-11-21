#include <Arduino.h>
#include <SPI.h>
#include <NRFLite.h>
#include "comm.h"
#include "blinkcodes.h"

NRFLite _radio; // The nrflite radio instance

bool clearTextMode = true;
bool blinkOnMessage = true;
uint8_t serialBuffer[SERIAL_BUFFER_SIZE];

/**
 * @brief Read data from the serial port
 *
 * @return int The number of bytes received, or 0 if no data was read
 */
uint8_t readDataFromSerial()
{
    static uint8_t received_bytes = 0;
    static bool messageStarted = false;
    static bool in_escape = false;

    while (Serial.available())
    {
        uint8_t byte = Serial.read();

        if (!in_escape && byte == ESCAPE_BYTE)
        {
            in_escape = true;
            continue;
        }

        if (!in_escape && byte == MESSAGE_SEPERATOR)
        {
            if (messageStarted) // If we have received bytes, we consider this the end of the message
            {
                uint8_t ret = received_bytes;
                messageStarted = false;
                serialBuffer[received_bytes] = '\0'; // Null terminate the buffer
                received_bytes = 0;
                //Serial.print("received_bytes: ");
                //Serial.println(ret);
                return ret;
            }
            received_bytes = 0; // Reset for a new message
            messageStarted = true;
            continue;
        }

        if (received_bytes < SERIAL_BUFFER_SIZE)
        {
            serialBuffer[received_bytes++] = byte;
        }
        in_escape = false;
    }
    return 0; // No Complete Packet
}

/**
 * @brief Wait for Host Connection and initialize the radio
 *
 */
void waitForHost()
{
    long lastInitMsg = -10000;
    digitalWrite(PIN_LED2, LOW);
    digitalWrite(PIN_LED2, LOW);

    while (true)
    {
        if (millis() - lastInitMsg > INIT_MSG_PERIOD)
        {
            sendInitMessage();
            lastInitMsg = millis();
        }
        delay(100); // Short delay to let the host read the message
        uint8_t serialDataSize = readDataFromSerial();
        if (serialDataSize == 0)
            continue; // Nothing received

        SerialMessage msg(serialBuffer, serialDataSize);
        if (msg.isValid() && msg.getdataSize() >= 2)
        {
            if (msg.getType() == MSG_TYPES::INIT) // Check if the packet type is INIT
            {
                uint8_t channel = msg.getData()[0];
                uint8_t address = msg.getData()[1];

                if (msg.getdataSize() >= 3)
                {
                    clearTextMode = msg.getData()[2];
                }
                if (msg.getdataSize() >= 4)
                {
                    blinkOnMessage = msg.getData()[3];
                }

                if (!testConnection(channel, address))
                {
                    sendStringMessage("NRF CONNECTION ERROR!", MSG_TYPES::ERROR);
                    delay(1000);
                    asm("JMP 0");
                }
                char msg[64];
                snprintf(msg, sizeof(msg), "NRF INITIALIZED channel:%d address:%d", channel, address);
                sendStringMessage(msg, MSG_TYPES::OK);
                break;
            }
            Serial.println((uint8_t)msg.getType());
        }
    }

    digitalWrite(PIN_LED2, HIGH);
    digitalWrite(PIN_LED2, HIGH);
}

void checkForSerialMsg()
{
    uint8_t serialDataSize = readDataFromSerial();
    if (serialDataSize == 0)
    {
        return; // Nothing received
    }

    SerialMessage msg(serialBuffer, serialDataSize);
    if (!msg.isValid())
    {
        Serial.println("Invalid Message!");
        msg.print();
        return;
    }

    if (msg.getType() == MSG_TYPES::REBOOT)
    {
        sendStringMessage("REBOOT...", MSG_TYPES::REBOOT);
        delay(100);
        asm("JMP 0");
    }
    else if (msg.getType() == MSG_TYPES::SETTING && msg.getdataSize() >= 1)
    {
        blinkOnMessage = msg.getData()[0];
    }
    else if (msg.getType() == MSG_TYPES::MSG && msg.getdataSize() >= 3)
    {
        uint8_t destinationID = msg.getData()[0];
        uint8_t requireAck = msg.getData()[1];
        nrfSend(destinationID, (void *)(msg.getData() + 2), msg.getdataSize() - 2, static_cast<bool>(requireAck));
    }
    else
    {
        Serial.println("Unknown Message!");
        msg.print();
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
    return (byte == MESSAGE_SEPERATOR || byte == ESCAPE_BYTE);
}

/**
 * @brief Sends an array of data bytes over the serial interface with byte stuffing.
 *        Special bytes  within the data are escaped to avoid confusion.
 *
 * @param data Pointer to the array of data bytes to be sent
 * @param size The number of data bytes to be sent from the array
 */
void sendPacketBS(uint8_t *data, uint8_t size, MSG_TYPES type)
{
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
}

/**
 * @brief Sends an array of data bytes over the serial interface in clear text mode.
 *
 * @param data Pointer to the array of data bytes to be sent
 * @param size The number of data bytes to be sent from the array
 */
void sendPacketClear(uint8_t *data, uint8_t size, MSG_TYPES type)
{
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
}

/**
 * @brief Sends an array of data bytes over the serial interface
 *
 * @param data Pointer to the array of data bytes to be sent
 * @param size The number of data bytes to be sent from the array
 */
void sendPacket(uint8_t *data, uint8_t size, MSG_TYPES type)
{
    Serial.write(MESSAGE_SEPERATOR);
    if (clearTextMode)
    {
        sendPacketClear(data, size, type);
    }
    else
    {
        sendPacketBS(data, size, type);
    }
    Serial.write(MESSAGE_SEPERATOR);
    Serial.write('\n');
    Serial.flush();
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
 * Sends the Message with byte Stuffing and with clear Text
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
