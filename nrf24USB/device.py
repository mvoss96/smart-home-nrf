import serial
import time
import threading
from threading import Lock
import queue
import logging
from enum import Enum
from typing import Optional

from .packet_reader import PacketReader, SpecialBytes, MSG_TYPES


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
        self.reader = PacketReader(self.serial_port)

    def start_read_loop(self):
        if self.read_thread is not None and self.read_thread.is_alive():
            self.stop_read_loop()
        self.read_thread = threading.Thread(target=self.read_loop, name="read_loop")
        self.read_thread.start()  # Start a new thread that runs the read_loop function

    def initialize_device(self):
        logging.info(f"Initialize NRF24USB Device with channel: {self.channel} address: {self.address} ...")

        # Try to Read the INIT message of the device
        (type, data) = self.reader.wait_for_packet(timeout=2.0)
        if type != MSG_TYPES.INIT or data == None or len(data) != 5:
            raise ConnectionError("Device did not send correct INIT message!")
        self.firmware_version = data[0]  # uint8_t
        self.serial_nr = data[1:5]  # 32-bit serial number
        logging.info(
            f"NRF24USBDevice reports Firmware version: {self.firmware_version} Serial: {':'.join(f'{x:02X}' for x in  self.serial_nr)}"
        )

        # Send the Host INIT message containing the channel and address
        self.serial_port.reset_input_buffer()  # Clear Input Buffer
        self.send_packet(bytes([self.channel, self.address]), MSG_TYPES.INIT)
        # Wait for the OK Message
        time.sleep(0.5)
        (type, _) = self.reader.wait_for_packet(timeout=1.0)
        if type != MSG_TYPES.OK:
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

    def send_msg(self, destination: int, data: list[int], require_ack=True) -> Optional[list[int]]:
        with self.lock:
            raw_msg_hex = [hex(i) for i in data]
            print(f"send to id:{destination} {raw_msg_hex}")

            self.send_packet(bytes([destination, require_ack]) + bytes(data), MSG_TYPES.MSG)
            if not require_ack:
                return []
            (recv_type, recv_data) = self.reader.wait_for_packet(timeout=0.5)
            if recv_type is MSG_TYPES.OK:
                return recv_data
            elif recv_type is MSG_TYPES.ERROR:
                return None
            else:
                logging.warning(f"Did not receive correct response to send_msg {recv_type} {recv_data}")
                return None

    def get_message(self):
        while True:
            try:
                (type, data) = self.msg_queue.get(block=True, timeout=0)
                if type is MSG_TYPES.MSG:
                    return data
            except queue.Empty:
                return None

    def handle_packet(self, type: Optional[int], packet_data: list[int]):
        try:
            packet_type = MSG_TYPES(type)
        except ValueError:
            logging.warning(f"Unsupported packet_type {type}!")
            return

        if packet_type == MSG_TYPES.ERROR:
            logging.error(
                f"NRF24USB Device reported ERROR: {bytes(packet_data).decode(errors='ignore') if packet_data is not None else ''}"
            )
        elif packet_type == MSG_TYPES.INIT:  # Device might have restarted attempt a reconnect
            logging.warning("NRF24USB Device appears to have reset!")
            self.connected = False
            self.initialize_device()
        else:
            logging.debug(f"Put packet in queque {packet_type} {packet_data}")
            self.msg_queue.put((packet_type, packet_data))  # Put the message into the queue

    def read_loop(self):
        logging.info("NRF24USB Read Loop Started!")
        try:
            while not self.stop_event.is_set():  # Inner loop for processing bytes
                with self.lock:
                    if not self.connected:
                        self.initialize_device()

                    if (pck := self.reader.read_packet()) is None:
                        continue
                    (packet_type, packet_data) = pck

                    if packet_type is not None:
                        logging.info(f"read_loop handle packet: {packet_type} {packet_data}")
                        if packet_type == MSG_TYPES.ERROR:
                            logging.error(
                                f"NRF24USB Device reported ERROR: {bytes(packet_data).decode(errors='ignore') if packet_data is not None else ''}"
                            )
                        elif packet_type == MSG_TYPES.INIT:  # Device might have restarted attempt a reconnect
                            logging.warning("NRF24USB Device appears to have reset!")
                            self.connected = False
                            self.initialize_device()
                        elif packet_type is not None:
                            logging.debug(f"put in loop {packet_type} {packet_data}")
                            self.msg_queue.put((packet_type, packet_data))  # Put the message into the queue
        except Exception as e:
            logging.exception(f"An exception occured during readLoop: {e}")

        logging.info("NRF24USB Read Loop Stopped!")

    def stop_read_loop(self):
        if self.read_thread != None:
            logging.info("Stopping Read Loop ...")
            self.stop_event.set()  # Set the stop event
            if threading.current_thread() != self.read_thread:
                self.read_thread.join()  # Wait for the thread to finish if called from another thread

    def __del__(self):
        self.stop_read_loop()
