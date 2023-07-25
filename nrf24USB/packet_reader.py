import serial
import logging
import time
from enum import Enum
from typing import Optional


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
        self.reset_buffer()

    def reset_buffer(self):
        self.in_escape = False
        self.data = []
        self.received_bytes = 0
        self.packet_type = None

    def wait_for_packet(self, timeout: float) -> tuple[Optional[MSG_TYPES], list]:
        start_time = time.time()
        self.reset_buffer()
        while time.time() - start_time < timeout:
            if (pck := self.read_packet()) is not None:
                return pck

        raise TimeoutError

    def read_packet(self) -> Optional[tuple[Optional[MSG_TYPES], list]]:
        try:
            if (waiting := self.port.in_waiting) > 0:
                byte = int.from_bytes(self.port.read(), byteorder='big') # Read Single Byte
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

            else:
                return None

        except serial.SerialException as e:
            logging.exception(f"A SerialException occured: {e}")
