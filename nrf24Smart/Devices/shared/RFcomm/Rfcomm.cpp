#include "RFcomm.h"
#include "config.h"
#include "status.h"
#include "blink.h"
#include <EEPROM.h>

uint8_t radioID = INITIAL_RADIO_ID;
uint8_t serverUUID[4];
bool serverConnected = false;
NRFLite _radio;
unsigned long statusTimer = 0;
volatile uint8_t _dataWasReceived;
static volatile bool _SendFailed = false;
static volatile bool _SendSuccess = false;

uint8_t ClientPacket::msgNum = 0;

void setStatus(const uint8_t *data, uint8_t length);

void radioInit()
{
#ifdef PIN_RADIO_POWER
    digitalWrite(PIN_RADIO_POWER, HIGH);
#endif
    Serial.print(F("Initialize Radio with ID:"));
    Serial.println(radioID);
    if (!_radio.init(radioID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS, RADIO_CHANNEL))
    {
        while (1)
        {
            Serial.println(F("Can't communicate with radio"));
#ifdef PIN_RADIO_POWER
            digitalWrite(PIN_RADIO_POWER, LOW);
#endif
            delay(1000);
        }
    }
    digitalWrite(PIN_RADIO_CE, LOW);
}

// Send fucntion with LED blink
uint8_t nrfSend(uint8_t toRadioId, void *data, uint8_t length, NRFLite::SendType sendType = NRFLite::REQUIRE_ACK)
{
    uint8_t res = 0;
    for (int i = 0; i < NUM_SEND_RETRIES; i++)
    {
        if (LED_BLINK_ONMESSAGE)
        {
            digitalWrite(PIN_LED1, LOW);
        }
#ifdef NRF_USE_IRQ
        _radio.startSend(toRadioId, data, length, sendType);
        while (!_SendSuccess && !_SendFailed)
        {
            // Wait for send to complete
        }
        res = _SendSuccess;
        _SendSuccess = false;
        _SendFailed = false;
        _radio.startRx();
#else
        res = _radio.send(toRadioId, data, length, sendType);
#endif
        if (LED_BLINK_ONMESSAGE)
        {
            digitalWrite(PIN_LED1, HIGH);
        }
        if (res == 1)
        {
            break;
        }
        Serial.print(F(" ... "));
        delay(200);
    }
    return res;
}

void sendStatus(bool isAck)
{
    ClientPacket pck(radioID, (isAck) ? MSG_TYPES::OK : MSG_TYPES::STATUS, (uint8_t *)&status, sizeof(status));
    if (pck.getInitialized())
    {
        Serial.print(millis());
        Serial.print(F(" <- Send status message with length "));
        Serial.print(pck.getSize());
        Serial.print(F(" "));
        // pck.printData();
        pck.print();
        if (nrfSend(SERVER_RADIO_ID, &pck, pck.getSize()))
        {
            Serial.println(F(" Success!"));
        }
        else
        {
            Serial.println(F(" Failed!"));
        }
    }
    else
    {
        Serial.println(F("ERROR: ClientPacket not initialized!"));
    }
    statusTimer = millis();
}

#ifdef IS_RF_REMOTE
void sendRemote(LAYERS layer, uint8_t value)
{
    if (status.targetID == 0)
    {
        Serial.println(F("Missing targetID"));
        return;
    }
    RemotePacket pck(radioID, status.targetUUID, layer, value);
    Serial.print(millis());
    Serial.print(F(" <- Send remote message to "));
    Serial.print(status.targetID);
    Serial.print(F(" with length "));
    Serial.print(pck.getSize());
    Serial.print(F(" "));
    pck.printData();
    pck.print();
    if (nrfSend(status.targetID, &pck, pck.getSize()))
    {
        Serial.println(F(" Success!"));
    }
    else
    {
        Serial.println(F(" Failed!"));
    }
}
#endif

void connectToServer()
{
    // Check if Server was already set up
    serverConnected = false;
    radioID = EEPROM.read(0);
    if (radioID != INITIAL_RADIO_ID)
    {
        serverUUID[0] = EEPROM.read(1);
        serverUUID[1] = EEPROM.read(2);
        serverUUID[2] = EEPROM.read(3);
        serverUUID[3] = EEPROM.read(4);
        // 5 and 6 reserved
        serverConnected = true;
    }

    if (!serverConnected)
    {
        radioInit();
        // Init package for handshake with sever
        ClientPacket pck(INITIAL_RADIO_ID, MSG_TYPES::INIT, (uint8_t *)DEVICE_TYPE, strlen(DEVICE_TYPE));
        while (!serverConnected)
        {
            // Try to reach Server
            Serial.println(F("Send INIT Message to Server!"));
            bool result = nrfSend(SERVER_RADIO_ID, &pck, pck.getSize());
            if (!result)
            {
                Serial.println(F("Server could not be reached for first INIT message!"));
                delay(3000);
                continue;
            }

            // Read answer message from server
            uint8_t buf[32] = {0};
            Serial.print(F("Wait for new ID from Server"));
            unsigned long startTime = millis();
            for (;;)
            {
                Serial.print(".");
                uint8_t packetSize = _radio.hasData();
                if (packetSize >= 5)
                {
                    Serial.println();
                    _radio.readData(&buf);
                    ServerPacket pck(buf, packetSize);
                    if (!pck.isValid())
                    {
                        Serial.println(F("ServerPacket not valid!"));
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
                    Serial.println(F(" Timeout while waiting for answer from Server!"));
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

bool checkUUID(ServerPacket pck)
{
    for (size_t i = 0; i < sizeof(serverUUID) / sizeof(serverUUID[0]); i++)
    {
        if (pck.getUUID()[i] != serverUUID[i])
        {
            return false;
        }
    }
    return true;
}

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
        _SendFailed = false;
    }
    else if (txFail)
    {
        _SendFailed = true;
        _SendSuccess = false;
    }
}

// Listen for new Messages
void listenForPackets()
{
    // Send Status at Interval
    if (statusInterval != 0 && millis() - statusTimer >= statusInterval * 1000)
    {
        sendStatus();
    }

#ifdef NRF_USE_IRQ
    if (_dataWasReceived == false)
        return;

    uint8_t packetSize = _radio.hasDataISR();
    _dataWasReceived = false;
#else
    uint8_t packetSize = _radio.hasData();
#endif

    uint8_t buf[32] = {0};
    _radio.readData(&buf); // Read the data into the buffer
    if (packetSize < 5)
    {
        Serial.println(F("WARNING: Invalid Packet size received!"));
        return;
    }

#ifdef ALLOW_REMOTE
    // Check if the message is a remote message
    if (buf[5] == (uint8_t)MSG_TYPES::REMOTE)
    {

        FromRemotePacket rPck(buf, packetSize);
        if (!rPck.isValid())
        {
            Serial.println(F("WARNING: Invalid RemotePacket received!"));
            return;
        }

        Serial.print(F("-> REMOTE message received "));
        rPck.print();
        Serial.println();
        setRemote(rPck.LAYER, rPck.VALUE);
        return;
    }
#endif
    // Assume message is a server message
    ServerPacket pck(buf, packetSize);
    if (!pck.isValid())
    {
        Serial.println(F("WARNING: Invalid Packet received!"));
        return;
    }
    if (!checkUUID(pck))
    {
        Serial.println(F("WARNING: Received Packet UUID mismatch!"));
        return;
    }

    if (LED_BLINK_ONMESSAGE)
    {
        digitalWrite(PIN_LED2, LOW);
    }
    switch ((MSG_TYPES)pck.getTYPE())
    {
    case MSG_TYPES::RESET:
        Serial.println(F("-> RESET message received..."));
        resetEEPROM();
        delay(1000);
        break;
    case MSG_TYPES::SET:
        Serial.print(F("-> SET message received "));
        pck.printData();
        Serial.println();
        setStatus(pck.getDATA(), pck.getSize());
        sendStatus(true);
        break;
    default:
        Serial.println(F("-> Unsupported message received!"));
    }
    if (LED_BLINK_ONMESSAGE)
    {
        digitalWrite(PIN_LED2, HIGH);
    }
}