import serial
import time
import threading
from threading import Lock
import queue
import logging
from typing import Optional

from .packet_reader import PacketReader, SpecialBytes, PACKET_TYPES


class NRF24Device:
    """
    Class for NRF24Device that provides functionalities to start read loop, initialize the device,
    send packets and messages, get messages, handle packets, read loop, stop read loop and destructor.
    """

    def __init__(
        self,
        port: str,
        channel: int,
        address: int,
        baudrate: int = 115200,
        use_clear_text: bool = False,
        blink_on_message: bool = True,
    ):
        """
        Initializes the NRF24Device with provided port, channel, address, and baudrate.
        Raises ValueError if the provided channel and address are out of allowed range.
        """
        if not 0 <= channel <= 125:
            raise ValueError("Channel must be between 0 and 125.")
        if not 0 <= address <= 255:
            raise ValueError("Address must be between 0 and 255.")

        self.lock = Lock()
        self.connected: bool = False
        self.channel: int = channel
        self.address: int = address
        self.stop_event = threading.Event()  # Create an event to signal the thread to stop
        self.msg_queue = queue.Queue()  # Create a new queue
        self.error: bool = False
        self.last_send_response: Optional[tuple] = None
        self.use_clear_text: bool = use_clear_text
        self.blink_on_message: bool = blink_on_message
        try:
            self.serial_port = serial.Serial(port, baudrate, timeout=None)
            self.read_thread = None
            self.reader = PacketReader(self.serial_port, use_clear_text)
        except Exception as err:
            logging.error(f"Could not connect to Serial Device {port}:{baudrate}")
            self.error = True

    def start_read_loop(self):
        """
        Starts the read loop in a new thread. If the read thread is already running, stops it before starting a new one.
        """
        if self.read_thread is not None and self.read_thread.is_alive():
            self.stop_read_loop()
        self.read_thread = threading.Thread(target=self.read_loop, name="read_loop")
        self.read_thread.start()  # Start a new thread that runs the read_loop function

    def initialize_device(self):
        """
        Initializes the NRF24USB device. Checks and sets the INIT message from the device. Raises ConnectionError if the device
        does not send correct INIT message or does not react correctly to Host INIT message.
        """
        logging.info(f"Initialize NRF24USB Device with channel: {self.channel} address: {self.address} ...")

        # Try to Read the INIT message of the device
        (type, data) = self.reader.wait_for_packet(timeout=3.0)
        if type != PACKET_TYPES.INIT or data == None or len(data) != 5:
            raise ConnectionError("Device did not send correct INIT message!")
        self.firmware_version = data[0]  # uint8_t
        self.serial_nr = data[1:5]  # 32-bit serial number
        logging.info(
            f"NRF24USBDevice reports Firmware version: {self.firmware_version} Serial: {':'.join(f'{x:02X}' for x in  self.serial_nr)}"
        )

        # Send the Host INIT message containing the channel and address
        self.serial_port.reset_input_buffer()  # Clear Input Buffer
        self.send_packet(bytes([self.channel, self.address, self.use_clear_text, self.blink_on_message]), PACKET_TYPES.INIT)
        # Wait for the OK Message
        time.sleep(0.5)
        (type, _) = self.reader.wait_for_packet(timeout=1.0)
        if type != PACKET_TYPES.OK:
            raise ConnectionError("Device did not react correctly to Host INIT message")
        logging.info("Device initialized successfully!")
        self.connected = True

    def _send_packet_bs(self, data: bytes, msg_type: PACKET_TYPES):
        """
        Sends packet to the NRF24USB device usig byte stuffing. Takes bytes of data and message type as arguments.
        """
        logging.info(f"sending packet bs {msg_type} {list(data)}")
        self.serial_port.write(bytes([msg_type.value]))  # Write the packet type
        for byte_value in data:
            if SpecialBytes.is_special_byte(byte_value):
                self.serial_port.write(bytes([SpecialBytes.ESCAPE_BYTE]))
                self.serial_port.write(bytes([byte_value]))
            else:
                self.serial_port.write(bytes([byte_value]))

    def _send_packet_clear(self, data: bytes, msg_type: PACKET_TYPES):
        """
        Sends packet to the NRF24USB device using clear text. Takes bytes of data and message type as arguments.
        """
        logging.info(f"sending packet clear {msg_type} {list(data)}")
        self.serial_port.write(msg_type.name.encode('utf-8'))
        for byte_value in data:
            self.serial_port.write(b":")
            self.serial_port.write(str((byte_value)).encode('utf-8'))

    def send_packet(self, data: bytes, msg_type: PACKET_TYPES):
        """
        Sends packet to the NRF24USB device. Takes bytes of data and message type as arguments.
        """ 
        self.serial_port.write(b";")
        if self.use_clear_text:
            self._send_packet_clear(data, msg_type)
        else:
            self._send_packet_bs(data, msg_type)
        self.serial_port.write(b";")


    def send_msg(self, destination: int, data: list[int], require_ack=True) -> Optional[list[int]]:
        """
        Sends message to the destination. Takes destination, list of integers as data, and acknowledgement requirement as arguments.
        Returns received data on successful acknowledgement or None otherwise.
        """

        # Make sure nrf24USB is connected
        if not self.connected:
            logging.warning("Waiting with send_msg while nrfDevice is not connected")
            while not self.connected:
                time.sleep(1)

        raw_msg_hex = [hex(i) for i in data]
        with self.lock:
            print(f"send to id:{destination} {raw_msg_hex}")
            self.send_packet(bytes([destination, require_ack]) + bytes(data), PACKET_TYPES.MSG)
        if not require_ack:
            return []

        # Wait for Device to respond
        start_time = time.time()
        self.last_send_response = None
        while self.last_send_response is None:
            if time.time() - start_time > 0.2:
                logging.error(f"Timeout while waiting for response from NRF24USB device")
                return None

        print("t: ", time.time() - start_time)
        try:
            (recv_type, recv_data) = self.last_send_response  # type: ignore
        except ValueError:
            logging.error(f"Cant unpack last_send_response: {self.last_send_response}")
            return None

        if recv_type is PACKET_TYPES.OK:
            return recv_data
        elif recv_type is PACKET_TYPES.ERROR:
            return None
        else:
            logging.error(f"Unknow recv_type: {recv_type}")

    def get_message(self):
        """
        Retrieves and returns the message from the queue. Returns None if the queue is empty.
        """
        while True:
            try:
                (type, data) = self.msg_queue.get(block=True, timeout=0)
                if type is PACKET_TYPES.MSG:
                    return data
            except queue.Empty:
                return None

    def handle_packet(self, packet_type: PACKET_TYPES, packet_data: list[int]):
        """
        Handles packet based on the type. Logs error for ERROR type, attempts to reconnect for INIT type,
        puts the message into the queue for MSG type, and logs warning for other types.
        """
        if packet_type == PACKET_TYPES.ERROR:
            logging.error(
                f"NRF24USB Device reported ERROR: {bytes(packet_data).decode(errors='ignore') if packet_data is not None else ''}"
            )
        elif packet_type == PACKET_TYPES.INIT:  # Device might have restarted attempt a reconnect
            logging.warning("NRF24USB Device appears to have reset!")
            self.connected = False
            self.initialize_device()
        elif packet_type == PACKET_TYPES.MSG:
            logging.debug(f"Put packet in queque {packet_type} {packet_data}")
            self.msg_queue.put((packet_type, packet_data))  # Put the message into the queue
        elif packet_type in (PACKET_TYPES.OK, PACKET_TYPES.ERROR):
            self.last_send_response = (packet_type, packet_data)
        else:
            logging.warning(f"MSG_TYPE {packet_type} {packet_data} was ignored!")

    def read_loop(self):
        """
        Starts the read loop for processing bytes until stop event is set. Handles the packets read by the reader.
        Logs an exception if any error occurs during the read loop.
        """
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
                        self.handle_packet(packet_type, packet_data)
        except Exception as e:
            logging.exception(f"An exception occured during readLoop: {e}")
            self.connected = False

        logging.info("NRF24USB Read Loop Stopped!")

    def stop_read_loop(self):
        """
        Stops the read loop if the read thread is not None. Waits for the thread to finish if called from another thread.
        """
        if self.read_thread != None:
            logging.info("Stopping Read Loop ...")
            self.stop_event.set()  # Set the stop event
            if threading.current_thread() != self.read_thread:
                self.read_thread.join()  # Wait for the thread to finish if called from another thread

    def __del__(self):
        """
        Stops the read loop and closes the serial port when the instance is destructed.
        """
        try:
            self.stop_read_loop()
            self.serial_port.close()
        except AttributeError:
            pass
