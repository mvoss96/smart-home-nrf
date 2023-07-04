import serial
import time
import threading
import queue
import logging
from enum import Enum
from typing import List


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


class NRF24Device:
    def __init__(self, port: str, channel: int, address: int):
        if not 0 <= channel <= 125:
            raise ValueError("Channel must be between 0 and 125.")
        if not 0 <= address <= 255:
            raise ValueError("Address must be between 0 and 255.")
        
        self.connected = False
        self.channel = channel
        self.address = address
        self.stop_event = threading.Event()  # Create an event to signal the thread to stop
        self.msg_queue = queue.Queue()  # Create a new queue
        self.serial_port = serial.Serial(port, 115200, timeout=None)
        self.read_thread = threading.Thread(target=self.read_loop)
        self.read_thread.start()  # Start a new thread that runs the read_loop function
        self.initialize_device()  # Attempt to initialize device

    def initialize_device(self):
        logging.info(f"initialize NRF24USB Device with channel: {self.channel} address: {self.address} ...")
        try:  # Try to Read the start message of the device
            (type, data) = self.msg_queue.get(block=True, timeout=2)
        except queue.Empty:
            self.stop_read_loop()
            raise ConnectionError("Device timeout on init!")

        if type is not None and data is not None:
            if type is not MSG_TYPES.INIT or not data:
                self.stop_read_loop()
                raise ConnectionError("Device did not send correct init message!")
        self.firmware_version = data[0]
        self.serial_nr = data[1:5]  # 32-bit serial number
        logging.info(f"Firmware version: {self.firmware_version} Serial: {':'.join(f'{x:02X}' for x in  self.serial_nr)}")
        self.send_packet(bytes([self.channel, self.address]), MSG_TYPES.INIT)  # Send the INIT message
        try:  # Try to Read the answer of the device
            (type, data) = self.msg_queue.get(block=True, timeout=1)
        except queue.Empty:
            self.stop_read_loop()
            raise ConnectionError("Device did not initialize!")
        if type is not MSG_TYPES.OK:
            raise ConnectionError("Device did not react correctly to init message")
        logging.info("Device initialized successfully!")
        self.connected = True

    def send_packet(self, data, msg_type):
        self.serial_port.write(bytes([SpecialBytes.START_BYTE]))  # Write the start byte
        self.serial_port.write(bytes([msg_type.value]))  # Write the packet type

        for byte in data:
            if SpecialBytes.is_special_byte(byte):
                self.serial_port.write(bytes([SpecialBytes.ESCAPE_BYTE]))
                self.serial_port.write(bytes([byte]))
            else:
                self.serial_port.write(bytes([byte]))
        self.serial_port.write(bytes([SpecialBytes.END_BYTE]))  # Write the end byte

    def send_msg(self, destination: int, data: List[int], require_ack=True):
        self.send_packet(bytes([destination, require_ack]) + bytes(data), MSG_TYPES.MSG)
        if not require_ack:
            return []
        while True:  # Read the answer of the device
            try:
                (type, data) = self.msg_queue.get(block=True, timeout=1)
            except queue.Empty:
                self.stop_read_loop()
                raise ConnectionError("Device did not answer")
            if type is MSG_TYPES.OK:
                try:  # Read Payload if available:
                    (type, payload) = self.msg_queue.get(block=True, timeout=0.5)
                    if type is MSG_TYPES.PAYLOAD:
                        return payload
                except queue.Empty:
                    return []
            elif type is MSG_TYPES.ERROR:
                return None
        
    def get_message(self):
        while True:
            try:
                (type, data) = self.msg_queue.get(block=True, timeout=0)
                if type is MSG_TYPES.MSG:
                    return data
            except queue.Empty:
                return None

    def read_loop(self):
        while not self.stop_event.is_set():  # Outer loop for maintaining connection
            try:
                data = []
                in_escape = False
                packet_type = None
                received_bytes = 0

                while not self.stop_event.is_set():  # Inner loop for processing bytes
                    num_available = self.serial_port.in_waiting
                    if num_available > 0:
                        byte_data = self.serial_port.read(num_available)
                        for byte in byte_data:
                            if not in_escape and byte == SpecialBytes.ESCAPE_BYTE:
                                in_escape = True
                                continue
                            if not in_escape:
                                if byte == SpecialBytes.START_BYTE:
                                    data = []  # Reset the data array when a new packet starts
                                    received_bytes = 0
                                    packet_type = None
                                    continue
                                if byte == SpecialBytes.END_BYTE:
                                    self.msg_queue.put((packet_type, data))  # Put the message into the queue
                                    continue
                            if received_bytes == 0 and packet_type == None:
                                try:
                                    packet_type = MSG_TYPES(byte)
                                except ValueError:
                                    pass
                            else:
                                data.append(byte)
                                received_bytes += 1
                            in_escape = False
                    else:
                        time.sleep(0.001)

            except serial.SerialException as e:
                logging.error(f"An exception occured during readLoop: {e}\nattempting reconnect...")
                self.initialize_device()


                
    def stop_read_loop(self):
        self.stop_event.set()  # Set the stop event
        self.read_thread.join()  # Wait for the thread to finish
        logging.info("Read Loop stopped!")

    def __del__(self):
        self.stop_read_loop()
