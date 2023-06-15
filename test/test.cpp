#include <boost/asio.hpp>
#include <iostream>
#include <unistd.h>
#include <iomanip>

#if defined(_WIN32)
#include <Windows.h>
#endif

using namespace boost;

class SpecialBytes
{
public:
    static const uint8_t START_BYTE = 0xFF;
    static const uint8_t END_BYTE = 0xFE;
    static const uint8_t ESCAPE_BYTE = 0xF0;

    static bool is_special_byte(uint8_t byte)
    {
        return byte == START_BYTE || byte == END_BYTE || byte == ESCAPE_BYTE;
    }
};

// Definition of static members
const uint8_t SpecialBytes::START_BYTE;
const uint8_t SpecialBytes::END_BYTE;
const uint8_t SpecialBytes::ESCAPE_BYTE;

int readDataFromSerial(boost::asio::serial_port &port, uint8_t *buffer, uint8_t bufferSize)
{
    static uint8_t received_bytes = 0;
    static bool in_escape = false;

    while (true)
    {
        boost::system::error_code ec;
        char c;
        boost::asio::read(port, boost::asio::buffer(&c, 1), ec);
        if (ec)
            return -1; // Some error occurred

        uint8_t byte = c;

        if (!in_escape && byte == SpecialBytes::ESCAPE_BYTE)
        {
            in_escape = true;
        }
        else
        {
            if (!in_escape)
            {
                if (byte == SpecialBytes::START_BYTE)
                {
                    received_bytes = 0;
                    continue;
                }
                if (byte == SpecialBytes::END_BYTE)
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
}

void send_packet(boost::asio::serial_port &port, const std::vector<uint8_t> &data, uint8_t msg_type)
{
    // Write the start byte
    boost::asio::write(port, boost::asio::buffer(&SpecialBytes::START_BYTE, 1));

    // Write the packet type
    boost::asio::write(port, boost::asio::buffer(&msg_type, 1));

    for (const auto &byte : data)
    {
        if (SpecialBytes::is_special_byte(byte))
        {
            // Write the escape byte
            boost::asio::write(port, boost::asio::buffer(&SpecialBytes::ESCAPE_BYTE, 1));
        }

        // Write the data byte
        boost::asio::write(port, boost::asio::buffer(&byte, 1));
    }

    // Write the end byte
    boost::asio::write(port, boost::asio::buffer(&SpecialBytes::END_BYTE, 1));
}

void readOneMsg(boost::asio::serial_port &port)
{
    uint8_t buffer[64];
    int result = readDataFromSerial(port, buffer, 64);
    if (result >= 0)
    {
        std::cout << "Received " << std::dec << result << " bytes.\n";
        for (int i = 0; i < result; ++i)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]) << " ";
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Error or timeout occurred.\n";
    }
}

void setDTR(boost::asio::serial_port &port, bool state)
{
#if defined(_WIN32)
    HANDLE handle = port.native_handle(); // Get the native handle
    // Get the device control block
    DWORD dcbSize = sizeof(DCB);
    DCB dcb;
    // Modify the DTR state
    if (state)
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
    else
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
#endif
}

int main()
{
    boost::asio::io_service io_service;
    boost::asio::serial_port port(io_service);
    try
    {
        port.open("/dev/ttyUSB0");
        port.set_option(boost::asio::serial_port::baud_rate(115200));
        //setDTR(port, true);
        send_packet(port,{0}, 6);
        readOneMsg(port);
        readOneMsg(port);
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }
    send_packet(port, {100, 0}, 2);
    readOneMsg(port);
    readOneMsg(port);

    port.close();
    return 0;
}
