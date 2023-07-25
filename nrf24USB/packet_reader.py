import serial
import logging
import time
from enum import Enum
from typing import Optional
from collections import deque


class MSG_TYPES(Enum):
    MSG = 0
    PAYLOAD = 1
    INIT = 2
    ERROR = 3
    OK = 4
    INFO = 5


class SpecialBytes:
    START_BYTE = 0xFF
    END_BYTE = 0xFE
    ESCAPE_BYTE = 0xF0
    ALL = {START_BYTE, END_BYTE, ESCAPE_BYTE}

    @classmethod
    def is_special_byte(cls, byte):
        return byte in cls.ALL


class PacketReader:
    def __init__(self, port: serial.Serial):
        self.port = port
        self.intermediate_buffer = deque()
        self.reset_buffer()

    def reset_buffer(self):
        """Resets the buffer and other related flags/counts for a new packet."""
        self.in_escape = False
        self.data = []
        self.received_bytes = 0
        self.packet_type = None

    def wait_for_packet(self, timeout: float) -> tuple[Optional[MSG_TYPES], list]:
        """
        Waits for a packet to be fully received.

        :param timeout: The maximum time to wait for a packet (in seconds).
        :raises TimeoutError: If a packet isn't received within the timeout.
        :return: The received packet's type and data as a tuple.
        """
        start_time = time.time()
        self.reset_buffer()
        while time.time() - start_time < timeout:
            if (pck := self.read_packet()) is not None:
                logging.info(f"time needed: {time.time() - start_time}")
                return pck

        raise TimeoutError
    
    def handle_byte(self, byte) -> Optional[tuple[Optional[MSG_TYPES], list]]:
        """
        Handles a received byte.

        :param byte: The received byte.
        :return: The received packet's type and data as a tuple, or None if packet isn't complete.
        """
        #print("read: ", byte)
        if not self.in_escape:  # Handle Special Bytes
            if byte == SpecialBytes.ESCAPE_BYTE:
                #print("escape byte")
                self.in_escape = True
                return None
            elif byte == SpecialBytes.START_BYTE:
                #print("start byte")
                self.reset_buffer()  # Reset the Data Array when a new packet starts
                return None
            elif byte == SpecialBytes.END_BYTE:
                #print("end byte")
                logging.info(f"Read Packet End: {self.packet_type} {self.data}")
                packet = (self.packet_type, self.data)
                self.reset_buffer()
                return packet

        if self.received_bytes == 0 and self.packet_type == None:  # First byte is packet type
            try:
                self.packet_type = MSG_TYPES(byte)
            except ValueError:
                logging.warning(f"Unsupported PackageType {byte}")
            
        else:
            self.data.append(byte)
            self.received_bytes += 1
        self.in_escape = False
        return None

    def read_packet(self) -> Optional[tuple[Optional[MSG_TYPES], list]]:
        """
        Reads a packet from the serial port.

        :return: The received packet's type and data as a tuple, or None if no packet is available.
        """
        if self.port.in_waiting > 0:
            byte_data = self.port.read_all()
            #print(byte_data)
            if byte_data is not None:
                self.intermediate_buffer.extend(byte_data)

        while len(self.intermediate_buffer) > 0:
            byte = self.intermediate_buffer.popleft()
            pck = self.handle_byte(byte)
            if pck is not None:
                return pck
        return None
