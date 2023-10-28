import serial
import logging
import time
from enum import Enum
from typing import Optional
from collections import deque
from threading import Lock


class PACKET_TYPES(Enum):
    NONE = 0
    OK = 1
    ERROR = 2
    MSG = 3
    INIT = 4
    SETTING = 5
    REBOOT = 6


class SpecialBytes:
    ESCAPE_BYTE = 0xF0
    BYTE_SEPERATOR = ord(b':')
    MESSAGE_SEPERATOR = ord(b';')
    ALL = {BYTE_SEPERATOR, MESSAGE_SEPERATOR, ESCAPE_BYTE}

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
        self.data_buffer = []
        self.received_bytes = 0
        self.in_escape = False
        self.packet_type = None
        self.message_started = False

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

    def parse_packet(self, buffer) -> Optional[tuple[Optional[PACKET_TYPES], list]]:
        """
        Parses the packet from a message
        :return: The received packet's type and data as a tuple, or None if no packet is available.
        """
        # Try finding a BYTE_SEPARATOR in the buffer to determine if it's a string-form message
        #print("parse: ", buffer)
        buffer_string = bytes(buffer).decode('utf-8', errors='ignore')
        #print("parse: ", buffer_string)
        separator_index = buffer_string.find(':') 

        if separator_index != -1:
            msg_type_str, remaining_str = buffer_string[:separator_index], buffer_string[separator_index + 1:]
            msg_type = getattr(PACKET_TYPES, msg_type_str, None)

            if msg_type is not None:
                # Tokenize the string and convert to bytes
                data = []
                for token in remaining_str.split(':'):
                    if not token:  # Skip empty tokens
                        continue
                    if not token.isdigit():
                        logging.warning("Invalid character in token")
                        return None  # Invalid character in token
                    value = int(token)
                    if value < 0 or value > 255:
                        logging.warning("Invalid value in token")
                        return None  # Invalid byte value
                    data.append(value)
                return msg_type, data

        # Fall through to byte-form message processing for unrecognized or non-string messages
        ord_type = buffer[0]
        try:
            msg_type = PACKET_TYPES(ord_type)
        except ValueError:
            logging.warning(f"Unknown packet type: {ord_type}")
            return None
        return msg_type, buffer[1:]
    

    # def check_parse_intermediate_buffer2(self) -> Optional[tuple[Optional[PACKET_TYPES], list]]:
    #     """
    #     Extracts and parses the packet from the serial intermediate buffer
    #     :return: The received packet's type and data as a tuple, or None if no packet is available.
    #     """
    #     buffer_str = bytes(self.intermediate_buffer).decode('utf-8', errors='ignore')
    #     print("buffer_str", buffer_str)

    #     # Check if there is a complete message enclosed by semicolons
    #     start_idx = buffer_str.find(';')
    #     end_idx = buffer_str.find(';', start_idx + 1)

    #     if start_idx != -1 and end_idx != -1:
    #         print(f"start_idx {start_idx} end_idx {end_idx}")
    #         # Extract complete message (not including semicolons)
    #         complete_msg = buffer_str[start_idx + 1:end_idx]
    #         print(f"complete_msg: {list(complete_msg)}")

    #         if not complete_msg.strip():
    #             # Handle the case where complete_msg is empty or contains only whitespace
    #             return None
            
    #         # Call your parse_msg function here
    #         pck = self.parse_packet(complete_msg)
    #         logging.info(pck)
            
    #         # Remove processed message (including semicolons) from intermediate buffer
    #         if len(self.intermediate_buffer) >= end_idx + 1:
    #             for _ in range(end_idx + 1):
    #                 self.intermediate_buffer.popleft()
            
    #         return pck

    #     return None
    
    def check_parse_intermediate_buffer(self) -> Optional[tuple[Optional[PACKET_TYPES], list]]:
        while len(self.intermediate_buffer) > 0:
            b = self.intermediate_buffer.popleft()  # This removes the leftmost element
            if not self.in_escape and b == SpecialBytes.ESCAPE_BYTE:
                self.in_escape = True
                #print("escape")
                continue

            if not self.in_escape and b == SpecialBytes.MESSAGE_SEPERATOR:
                if self.message_started: # End Of Message
                    self.message_started = False
                    #print("End Of Message")
                    return self.parse_packet(self.data_buffer)
                
                #print("start Of Message")
                self.message_started = True
                self.data_buffer = [] # Start Of Message
                continue

            self.data_buffer.append(b)
            self.in_escape = False
        
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

                pck  =  self.check_parse_intermediate_buffer()
                if pck is not None:
                    logging.info(pck)
                    return pck
            return None
