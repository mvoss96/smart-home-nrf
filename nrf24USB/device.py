import serial
import time
import threading
from threading import Lock
import queue
import logging
from enum import Enum
from typing import Optional, List, Tuple


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
    def __init__(self, port: str, channel: int, address: int, baudrate=115200):
        if not 0 <= channel <= 125:
            raise ValueError("Channel must be between 0 and 125.")
        if not 0 <= address <= 255:
            raise ValueError("Address must be between 0 and 255.")
        
        self.lock = Lock()
        self.connected = False
        self.channel = channel
        self.address = address
        self.stop_event = threading.Event()  # Create an event to signal the thread to stop
        self.msg_queue = queue.Queue()  # Create a new queue
        self.serial_port = serial.Serial(port, baudrate, timeout=None)
        self.read_thread = None

    def start_read_loop(self):
        if self.read_thread is not None and self.read_thread.is_alive():
            self.stop_read_loop()
        self.read_thread = threading.Thread(target=self.read_loop, name = "read_loop")
        self.read_thread.start()  # Start a new thread that runs the read_loop function

    def initialize_device(self):
        logging.info(f"Initialize NRF24USB Device with channel: {self.channel} address: {self.address} ...")
        # Try to Read the INIT message of the device
        (type, data) = self.read_packet(timeout=2)
        if type != MSG_TYPES.INIT:
            raise ConnectionError("Device did not send correct INIT message!")
        if data == None or len(data) != 5:
            raise ConnectionError("Device did not send correct INIT data!")
        self.firmware_version = data[0] # uint8_t
        self.serial_nr = data[1:5]  # 32-bit serial number
        logging.info(
            f"NRF24USBDevice reports Firmware version: {self.firmware_version} Serial: {':'.join(f'{x:02X}' for x in  self.serial_nr)}"
        )
        # Send the Host INIT message containing the channel and address
        self.send_packet(bytes([self.channel, self.address]), MSG_TYPES.INIT) 
        # Try to Read the answer of the device
        (type, data) = self.read_packet(timeout=2)
        if type is not MSG_TYPES.OK:
            raise ConnectionError("Device did not react correctly to Host INIT message")
        logging.info("Device initialized successfully!")
        self.connected = True

    def send_packet(self, data: bytes, msg_type: MSG_TYPES):
        logging.debug(f"sending packet {msg_type} {data}")
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
        with self.lock:
            self.send_packet(bytes([destination, require_ack]) + bytes(data), MSG_TYPES.MSG)
            if not require_ack:
                return []
            time.sleep(0.01) # Wait for aswer form NRf24USB Device
            (recv_type, recv_data) = self.read_packet(timeout=0.5)
            if recv_type is MSG_TYPES.OK:
                return recv_data
            elif recv_type is MSG_TYPES.ERROR:
                return None
            else:
                raise ConnectionError(f"Device did not answer {recv_type}")

    def get_message(self):
        while True:
            try:
                (type, data) = self.msg_queue.get(block=True, timeout=0)
                if type is MSG_TYPES.MSG:
                    return data
            except queue.Empty:
                return None
            

    def read_packet(self, timeout: float = 2.0) -> Tuple[Optional[MSG_TYPES], Optional[List[int]]]:
        in_escape = False
        packet_type = None
        data = []
        received_bytes = 0
        start_time = time.time()
        while True:  
            try:
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
                                logging.info(f"read packet: {packet_type} {data}")
                                return (packet_type, data)
                        if received_bytes == 0 and packet_type == None:
                            try:
                                packet_type = MSG_TYPES(byte)
                            except ValueError:
                                packet_type = None
                        else:
                            data.append(byte)
                            received_bytes += 1
                        in_escape = False
                else:
                    time.sleep(0.001)

                if time.time() - start_time > timeout:
                    return (None, None)

            except serial.SerialException as e:
                logging.exception(f"A Serial Exception occured: {e}")

    
    def read_loop(self):
        logging.info("NRF24USB Read Loop Started!")
        try:
            while not self.stop_event.is_set():  # Inner loop for processing bytes
                with self.lock:
                    if not self.connected:
                        self.initialize_device()
                    (packet_type, packet_data) = self.read_packet(timeout=0)
                    if packet_type == MSG_TYPES.ERROR:
                        logging.error(f"NRF24USB Device reported ERROR: {packet_data}")
                    elif packet_type == MSG_TYPES.INIT: # Device might have restarted attempt a reconnect
                        logging.warning("NRF24USB Device appears to have reset!")
                        self.connected = False
                        self.initialize_device()
                    elif packet_type is not None:
                        logging.debug(f"put in loop {packet_type} {packet_data}")
                        self.msg_queue.put((packet_type, packet_data))  # Put the message into the queue
        except Exception as e:
            logging.exception(f"An exception occured during readLoop: {e}")
            
        logging.info("NRF24USB Read Loop Stopped!")
                

    # def read_loop2(self):
    #     logging.info("NRF24USB Read Loop Started!")
    #     data = []
    #     in_escape = False
    #     packet_type = None
    #     received_bytes = 0
    #     while not self.stop_event.is_set():  # Inner loop for processing bytes
    #         try:
    #             if not self.connected:
    #                 self.initialize_device()
    #             num_available = self.serial_port.in_waiting
    #             if num_available > 0:
    #                 byte_data = self.serial_port.read(num_available)
    #                 for byte in byte_data:
    #                     if not in_escape and byte == SpecialBytes.ESCAPE_BYTE:
    #                         in_escape = True
    #                         continue
    #                     if not in_escape:
    #                         if byte == SpecialBytes.START_BYTE:
    #                             data = []  # Reset the data array when a new packet starts
    #                             received_bytes = 0
    #                             packet_type = None
    #                             continue
    #                         if byte == SpecialBytes.END_BYTE:
    #                             if packet_type != None:
    #                                 self.parse_msg(packet_type, data)
    #                             continue
    #                     if received_bytes == 0 and packet_type == None:
    #                         try:
    #                             packet_type = MSG_TYPES(byte)
    #                         except ValueError:
    #                             pass
    #                     else:
    #                         data.append(byte)
    #                         received_bytes += 1
    #                     in_escape = False
    #             else:
    #                 time.sleep(0.001)

    #         except Exception as e:
    #             logging.exception(f"An exception occured during readLoop: {e}")
    #             break

    #     logging.info("NRF24USB Read Loop Stopped!")

    def stop_read_loop(self):
        if self.read_thread != None:
            logging.info("Stopping Read Loop ...")
            self.stop_event.set()  # Set the stop event
            if threading.current_thread() != self.read_thread:
                self.read_thread.join()  # Wait for the thread to finish if called from another thread

    def __del__(self):
        self.stop_read_loop()
