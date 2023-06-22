from nrf24Smart import DeviceMessage, HostMessage, MSG_TYPES, SetMessage, CHANGE_TYPES, supported_devices
import time
from DeviceManager import DeviceManager
import logging

# Configure the root logger
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class CommunicationManager:
    def __init__(self, device_manager: DeviceManager):
        # Initializing the last_update_time
        self.last_update_time = time.time()

        # Reference to the DeviceManager instance to handle device operations
        self.device_manager = device_manager

    def handle_status_message(self, msg: DeviceMessage):
        """
        Updates the status of a device given a DeviceMessage.
        The method fetches the device from the database, checks its type and updates its status.
        """
        device = self.device_manager.db_manager.search_device_in_db(msg.UUID)
        if device == None:
            logger.error(f"Device with uuid:{msg.UUID} not in DB!")
            return

        # Check if the class exists in the supported_devices list
        class_obj = self.device_manager.get_supported_device(device["type"])
        if class_obj == None:
            logger.error(f"Unsupported Device of type:{device['type']} in DB!")
            return
        # Create an instance of the class
        try:
            instance = class_obj(msg.DATA)
            
            # Update the status key for the device using the device variable
            if device["battery_powered"]:
                device["battery_level"] = msg.BATTERY
            device["status"] = instance.get_status()
            device["last_seen"] = time.strftime("%Y-%m-%d %H:%M:%S")
            self.device_manager.db_manager.update_device_in_db(device)
        except Exception as err:
            logging.error(err)

    def handle_boot_message(self, msg: DeviceMessage):
        """
        Checks the BOOT message from a device.
        It validates that the UUID of the server corresponds to the stored UUID reported by the Device
        """
        logger.info(f"BOOT message from device:{msg.UUID}")
        if msg.DATA != self.device_manager.db_manager.uuid:
            logger.warning(
                f"Mismatched UUID! Device with uuid:{msg.UUID} reports server uuid:{msg.DATA} instead of {self.device_manager.db_manager.uuid}"
            )

    def handle_init_mesage(self, msg: DeviceMessage):
        """
        Initializes a new Device with the device_manager
        """
        self.device_manager.init_new_device(msg)
        self.poll_device(msg.UUID)

    def listen(self):
        """
        Listens for incoming messages.
        If a message is available, it processes the message according to its type.
        """
        data = self.device_manager.get_device_message()
        if data:
            # print(data)
            msg = DeviceMessage(data)
            if not msg.is_valid:
                logger.warning("invalid message!")
                return
            if msg.MSG_TYPE == MSG_TYPES.INIT.value:
                self.handle_init_mesage(msg)
            elif msg.MSG_TYPE == MSG_TYPES.BOOT.value:
                self.handle_boot_message(msg)
            elif msg.MSG_TYPE == MSG_TYPES.STATUS.value:
                self.handle_status_message(msg)
            else:
                logger.info("->", msg)

        else:
            time.sleep(0.01)

    def poll_all_devices(self):
        """
        Updates the status of all devices if at least one second has passed since the last update.
        """
        current_time = time.time()
        if current_time - self.last_update_time < 2.0:  # less than one second has passed
            return  # Return without doing anything
        for item in self.device_manager.db_manager.get_all_devices():
            # print(f"update status of device:{item['type']} uuid:{item['uuid']}")
            self.poll_device(item["uuid"])

        self.last_update_time = current_time  # Update last_run_time to current_time

    def poll_device(self, uuid: list[int]):
        """
        Updates a device's status by sending a GET message
        The device should then respond with a STATUS message
        Battery powered devices can't be polled.
        """
        device = self.device_manager.db_manager.search_device_in_db(uuid)
        if device is None:
            logger.error(f"Device with uuid:{uuid} not in DB!")
            return
        if device["battery_powered"] == True:
            # Battery powered devices cant be polled.
            return
        msg = HostMessage(uuid=self.device_manager.db_manager.uuid, msg_type=MSG_TYPES.GET, data=[])
        if self.device_manager.send_msg_to_device(device["id"], msg.get_raw()) == None:
            logger.error(f"Failed to send GET message to device:{device['type']} with uuid:{device['uuid']}!")

    def set_device_param(self, uuid: list[int], parameter: str, new_val: str) -> bool:
        """
        Send a SET message to a device to change a status parameter.
        Returns False if no SET message could be created for the given parameter and value
        """
        device = self.device_manager.db_manager.search_device_in_db(uuid)
        if device is None:
            logger.error(f"Device with uuid:{uuid} not in DB!")
            return False
        class_obj = self.device_manager.get_supported_device(device["type"])
        if class_obj is None:
            return False
        set_msg = class_obj.set_parameter(parameter, new_val)
        if set_msg is None:
            return False
        msg = HostMessage(
            uuid=self.device_manager.db_manager.uuid,
            msg_type=MSG_TYPES.SET,
            data=set_msg.get_raw(),
        )
        if self.device_manager.send_msg_to_device(device["id"], msg.get_raw()) == None:
            logger.error(f"Failed to send SET message to device:{device['type']} with uuid:{device['uuid']}!")
        self.poll_device(uuid)
        return True
