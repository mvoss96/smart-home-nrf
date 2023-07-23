#include <SPI.h>
#include <NRFLite.h>
#include <EEPROM.h>
#include "RFcomm.h"
#include "blink.h"
#include "control.h"

uint8_t radioID = INITIAL_RADIO_ID;
uint8_t serverUUID[4];
bool serverConnected = false;
NRFLite _radio;

// Send fucntion with LED blink
uint8_t nrfSend(uint8_t toRadioId, void *data, uint8_t length, NRFLite::SendType sendType = NRFLite::REQUIRE_ACK)
{
    if (LED_BLINK_ONMESSAGE)
    {
        blink(PIN_LED1, 1, 100);
    }
    return _radio.send(SERVER_RADIO_ID, data, length, sendType);
}

void resetEEPROM()
{
    Serial.println("Reset EEPROM...");
    blinkBoth(6, 200);
    for (size_t i = 0; i < EEPROM.length(); i++)
    {
        EEPROM.update(i, INITIAL_RADIO_ID);
    }
    serverConnected = false;
    radioID = INITIAL_RADIO_ID;
    serverUUID[0] = 0;
    serverUUID[1] = 0;
    serverUUID[2] = 0;
    serverUUID[3] = 0;
}

void printEEPROM(int n)
{
    if (n <= 0)
    {
        n = EEPROM.length();
    }
    for (int i = 0; i < n; i++)
    {
        uint8_t val = EEPROM.read(i);
        Serial.print("Address: ");
        Serial.print(i);
        Serial.print(" Value: ");
        Serial.println(val, HEX);
    }
}

void loadFromEEPROM()
{
    // Check if Server was already set up and new ID saved
    radioID = EEPROM.read(0);
    if (radioID != INITIAL_RADIO_ID)
    {
        serverUUID[0] = EEPROM.read(1);
        serverUUID[1] = EEPROM.read(2);
        serverUUID[2] = EEPROM.read(3);
        serverUUID[3] = EEPROM.read(4);
        serverConnected = true;
        return;
    }
    serverConnected = false;
}

void saveToEEPROM()
{
    blinkBoth(2, 200);
    EEPROM.update(0, radioID);
    EEPROM.update(1, serverUUID[0]);
    EEPROM.update(2, serverUUID[1]);
    EEPROM.update(3, serverUUID[2]);
    EEPROM.update(4, serverUUID[3]);
}

void radioInit()
{
    Serial.print("Initialize Radio with ID:");
    Serial.println(radioID);
    if (!_radio.init(radioID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS, RADIO_CHANNEL))
    {
        while (1)
        {
            Serial.println("Can't communicate with radio");
            delay(1000);
        }
    }
}

void connectToServer()
{
    // Check if Server was already set up
    serverConnected = false;
    loadFromEEPROM();
    if (!serverConnected)
    {
        radioInit();
        // Init package for handshake with sever
        ClientPacket pck(INITIAL_RADIO_ID, MSG_TYPES::INIT, (uint8_t *)DEVICE_TYPE, strlen(DEVICE_TYPE));
        while (!serverConnected)
        {
            // Try to reach Server
            Serial.println("Send INIT Message to Server!");
            bool result = nrfSend(SERVER_RADIO_ID, &pck, pck.getSize());
            if (!result)
            {
                Serial.println("Server could not be reached for first INIT message!");
                delay(3000);
                continue;
            }

            // Read answer message from server
            uint8_t buf[32] = {0};
            Serial.print("Wait for new ID from Server");
            unsigned long startTime = millis();
            for (;;)
            {
                Serial.print(".");
                if (uint8_t packetSize = _radio.hasData(); packetSize >= 5)
                {
                    Serial.println();
                    _radio.readData(&buf);
                    ServerPacket pck(buf, packetSize);
                    if (!pck.isValid())
                    {
                        Serial.println("ServerPacket not valid!");
                        break;
                    }
                    uint8_t newID = pck.getDATA()[0];
                    Serial.print("New ID: ");
                    Serial.println(newID);
                    radioID = newID;
                    memcpy(serverUUID, pck.getUUID(), 4);
                    serverConnected = true;
                    saveToEEPROM();
                    break;
                }
                if (millis() - startTime > 2000)
                {
                    Serial.println(" Timeout while waiting for answer from Server!");
                    break;
                }
                delay(500);
            }
        }
    }
    // Reinitialize the rado with the new ID
    radioInit();
    // Send BOOT message
    ClientPacket bootpck(radioID, MSG_TYPES::BOOT, serverUUID, 4);
    nrfSend(SERVER_RADIO_ID, &bootpck, bootpck.getSize());
}

void sendStatus()
{
    ClientPacket pck(radioID, MSG_TYPES::STATUS, (uint8_t *)&status, sizeof(status));
    if (pck.getInitialized())
    {
        nrfSend(SERVER_RADIO_ID, &pck, pck.getSize());
    }
    else
    {
        Serial.println("Could not send not initialized ClientPacket!");
    }
}