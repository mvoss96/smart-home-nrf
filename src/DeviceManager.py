from nrf24USB import NRF24Device
from nrf24Smart import DeviceMessage, HostMessage, MSG_TYPES, supported_devices, DeviceStatus
import time
from typing import Type, Optional
from src.DBManager import DBManager
from threading import Lock
from src.Logger import setup_logger

logger = setup_logger()

class DeviceManager:
    def __init__(self, db_manager: DBManager):
        # Initializing the NRF24Device
        self.device = NRF24Device("/dev/NRF24USB", channel=101, address=0)
        #self.device = NRF24Device("COM3", channel=101, address=0)

        # Initialize the lock
        self.comm_lock = Lock()

        # Reference to the DBManager instance to handle DB operations
        self.db_manager = db_manager

    def start(self):
        self.device.start_read_loop()
        # Wait until the Device has been initialized
        start_time = time.time()
        while not self.device.connected:
            if time.time() - start_time > 5:
                raise TimeoutError("NRF24Device could not be started")

    def get_supported_device(self, device_type: str) -> Optional[Type[DeviceStatus]]:
        # Check if the class exists in the supported_devices list
        if not any(hasattr(cls, "__name__") and cls.__name__ == device_type for cls in supported_devices):
            logger.error(f"Device type {device_type} not supported!")
            return None
        # Get the class object by name
        return next((cls for cls in supported_devices if cls.__name__ == device_type))

    def send_msg_to_device(self, device_id: int, raw_msg: list[int], require_ack = True):
        """
        Sends a message to a device given its device ID and the raw message data.
        """
        with self.comm_lock:
            return self.device.send_msg(device_id, raw_msg, require_ack)

    def init_new_device(self, msg: DeviceMessage):
        """
        Given a DeviceMessage, it initializes a new device.
        The device type is checked, if a device with the UUID exists and gets a new ID, and the new device is added to the DB.
        """
        device_type = bytes(msg.DATA).decode(errors="ignore")
        logger.info(f"New Device: {device_type} {msg}")

        # Check if the class exists in the supported_devices list
        if self.get_supported_device(device_type) == None:
            return

        # Check if a device with the UUID exists
        result = self.db_manager.search_device_in_db(msg.UUID)
        if result:
            logger.warn(f"Device with uuid:{msg.UUID} already in DB!")
            return

        # Get smallest free id between 1 and 254:
        new_id = self.db_manager.get_free_id()
        if new_id is None:
            logger.error("No free ID available")
            return
        logger.info(f"New device ID:{new_id}")

        # Send new ID to device
        to_send_msg = HostMessage(uuid=self.db_manager.uuid, msg_type=MSG_TYPES.INIT, data=[new_id])
        logger.info(f"Sending new ID to {msg.ID} with data: {to_send_msg.get_raw()}")
        if self.send_msg_to_device(msg.ID, to_send_msg.get_raw()) == None:
            logger.error(f"Failed to initialize device {device_type}")
            return

        # Add new Device
        self.db_manager.add_device_to_db(
            {
                "uuid": msg.UUID,
                "id": new_id,
                "version": msg.FIRMWARE_VERSION,
                "battery_powered": msg.BATTERY != 0,
                "battery_level": 255,
                "type": device_type,
                "name": device_type,
                "last_seen": time.strftime("%Y-%m-%d %H:%M:%S"),
            }
        )

    def get_device_message(self):
        """
        Get a device message from the NRF24Device.
        The method returns None if no message is available.
        """
        with self.comm_lock:
            return self.device.get_message()
