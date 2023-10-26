import serial
import logging
import time
from enum import Enum
from typing import Optional
from collections import deque
from threading import Lock


class PACKET_TYPES(Enum):
    MSG = 0
    PAYLOAD = 1
    INIT = 2
    ERROR = 3
    OK = 4
    EMPTY = 5
    REBOOT = 6
    SETTING = 7


class SpecialBytes:
    START_BYTE = 0xFF
    END_BYTE = 0xFE
    ESCAPE_BYTE = 0xF0
    ALL = {START_BYTE, END_BYTE, ESCAPE_BYTE}

    @classmethod
    def is_special_byte(cls, byte):
        return byte in cls.ALL


class PacketReader:
    def __init__(self, port: serial.Serial, use_clear_text: bool = False):
        self.port = port
        self.intermediate_buffer = deque()
        self.use_clear_text = use_clear_text
        self.reset_buffer()
        self.lock = Lock()
        self.packet_lock = Lock()

    def reset_buffer(self):
        """Resets the buffer and other related flags/counts for a new packet."""
        self.in_escape = False
        self.text_buffer = ""
        self.data = []
        self.received_bytes = 0
        self.packet_type = None

    def wait_for_packet(self, timeout: float) -> tuple[Optional[PACKET_TYPES], list]:
        """
        Waits for a packet to be fully received.

        :param timeout: The maximum time to wait for a packet (in seconds).
        :raises TimeoutError: If a packet isn't received within the timeout.
        :return: The received packet's type and data as a tuple.
        """
        with self.lock:
            start_time = time.time()
            self.reset_buffer()
            while time.time() - start_time < timeout:
                if (pck := self.read_packet()) is not None:
                    logging.info(f"time needed: {time.time() - start_time}")
                    return pck

            raise TimeoutError

    def handle_byte(self, byte) -> Optional[tuple[Optional[PACKET_TYPES], list]]:
        """
        Handles a received byte.

        :param byte: The received byte.
        :return: The received packet's type and data as a tuple, or None if packet isn't complete.
        """
        # print("read: ", byte)
        if self.use_clear_text:
            return self.handle_byte_clear_text(byte)

        if not self.in_escape:  # Handle Special Bytes
            if byte == SpecialBytes.ESCAPE_BYTE:
                # print("escape byte")
                self.in_escape = True
                return None
            elif byte == SpecialBytes.START_BYTE:
                # print("start byte")
                self.reset_buffer()  # Reset the Data Array when a new packet starts
                return None
            elif byte == SpecialBytes.END_BYTE:
                if self.packet_type is None:
                    logging.warning(f"END_BYTE received before START_BYTE.")
                    return None
                # print("end byte")
                logging.info(f"Read Packet End: {self.packet_type} {self.data}")
                packet = (self.packet_type, self.data)
                self.reset_buffer()
                return packet

        if self.received_bytes == 0 and self.packet_type == None:  # First byte is packet type
            try:
                self.packet_type = PACKET_TYPES(byte)
            except ValueError:
                logging.warning(f"Unsupported PackageType {byte}")

        else:
            self.data.append(byte)
            self.received_bytes += 1
        self.in_escape = False
        return None

    def handle_byte_clear_text(self, byte) -> Optional[tuple[Optional[PACKET_TYPES], list]]:
        """
        Handles a received byte using the clear text protocoll.

        :param byte: The received byte.
        :return: The received packet's type and data as a tuple, or None if packet isn't complete.
        """
        if byte == ";":
            self.reset_buffer()
        elif byte == "\r":
            return
        if byte == "\n":
            tokens = self.text_buffer.split(":")
            print(tokens)
            if len(tokens) == 0:
                return None

            try:
                self.packet_type = PACKET_TYPES(tokens[0])
                for t in tokens:
                    self.data.append(int(t))
            except ValueError:
                logging.warning("Malformed clear text sequence!", tokens)
                return None

            packet = (self.packet_type, self.data)
            self.reset_buffer()
            return packet
        else:
            self.text_buffer += byte
        return None

    def read_packet(self) -> Optional[tuple[Optional[PACKET_TYPES], list]]:
        """
        Reads a packet from the serial port.

        :return: The received packet's type and data as a tuple, or None if no packet is available.
        """
        with self.packet_lock:
            if self.port.in_waiting > 0:
                byte_data = self.port.read_all()
                # print(byte_data)
                if byte_data is not None:
                    self.intermediate_buffer.extend(byte_data)

            while len(self.intermediate_buffer) > 0:
                byte = self.intermediate_buffer.popleft()
                pck = self.handle_byte(byte)
                if pck is not None:
                    return pck
            return None
