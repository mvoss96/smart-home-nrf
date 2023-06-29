from nrf24Smart import DeviceMessage, HostMessage, MSG_TYPES, SetMessage, CHANGE_TYPES, supported_devices
import time
from datetime import datetime
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
        self.db_manager = self.device_manager.db_manager

    def handle_status_message(self, msg: DeviceMessage):
        """
        Updates the status of a device given a DeviceMessage.
        The method fetches the device from the database, checks its type and updates its status.
        """
        device = self.db_manager.search_device_in_db(msg.UUID)
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
            self.db_manager.update_device_in_db(device)
        except Exception as err:
            logging.error(err)

    def handle_boot_message(self, msg: DeviceMessage):
        """
        Checks the BOOT message from a device.
        It validates that the UUID of the server corresponds to the stored UUID reported by the Device
        """
        logger.info(f"BOOT message from device:{msg.UUID}")
        if msg.DATA != self.db_manager.uuid:
            logger.warning(
                f"Mismatched UUID! Device with uuid:{msg.UUID} reports server uuid:{msg.DATA} instead of {self.db_manager.uuid}"
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
        while True:
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

            time.sleep(0.01)

    def update_all_devices(self):
        """
        Send any pending status changes in set_status to the devices.
        Poll all devices if at least one second has passed since the last update.
        """
        while True:
            current_time = time.time()
            # if current_time - self.last_update_time < 0.5:
            #     return  # Return without doing anything if called to soon
            for device in self.db_manager.get_all_devices():
                device_copy = dict(device)  # Create a deep copy to prevent issues when another thread changes the db
                uuid = device["uuid"]
                # print(f"update status of device:{device['type']} uuid:{uuid}")
                if "set_status" in device_copy and device_copy.get("set_status"):
                    if (class_obj := self.device_manager.get_supported_device(device_copy["type"])) == None:
                        logging.error("Database contains not supported device")
                        continue
                    keys_to_remove = []
                    for key, value in device_copy["set_status"].items():
                        print(f"sending set_status param {key} {value}")
                        if (set_message := class_obj.create_set_message(key, value)) == None:
                            logging.error(f"Database contains not supported set_message parameters {key}: {value}")
                            keys_to_remove.append(key)
                            continue

                        # Skipping redundant status changes
                        # test_val = set_message.newValue[0] if len(set_message.newValue) > 0 else None
                        # if key in device_copy["status"] and test_val == device_copy["status"][key]:
                        #     logging.info(f"Skipped status update {key} {value} for device {uuid}")
                        #     keys_to_remove.append(key)
                        # continue

                        msg = HostMessage(
                            uuid=self.db_manager.uuid, msg_type=MSG_TYPES.SET, data=set_message.get_raw()
                        )
                        res = self.device_manager.send_msg_to_device(device_copy["id"], msg.get_raw())
                        if res == None:
                            logging.error(
                                f"Failed to send SET message to device:{device_copy['type']} with uuid:{device_copy['uuid']}!"
                            )
                        else:
                            keys_to_remove.append(key)

                        time.sleep(0.2)
                    self.poll_device(uuid)

                    for key in keys_to_remove:
                        # Only remove if values have not changed:
                        try:
                            if device_copy["set_status"][key] == device["set_status"][key]:
                                # print(f"delete {key} {device_copy['set_status'][key]} {device['set_status'][key]}")
                                del device["set_status"][key]
                        except KeyError:
                            pass
                    self.db_manager.update_device_in_db(device)

                try:
                    timestamp = datetime.strptime(device["status"]["timestamp"], "%Y-%m-%d %H:%M:%S")
                    elapsed_time = current_time - timestamp.timestamp()
                    if elapsed_time > 5:
                        self.poll_device(uuid)
                except KeyError:
                    pass

            time.sleep(0.5)

    def poll_device(self, uuid: list[int]):
        """
        Updates a device's status by sending a GET message
        The device should then respond with a STATUS message
        Battery powered devices can't be polled.
        """
        device = self.db_manager.search_device_in_db(uuid)
        if device is None:
            logger.error(f"Device with uuid:{uuid} not in DB!")
            return
        if device["battery_powered"] == True:
            # Battery powered devices cant be polled.
            return
        logging.info(f"poll device {uuid}")
        msg = HostMessage(uuid=self.db_manager.uuid, msg_type=MSG_TYPES.GET, data=[])
        for i in range(2):
            if self.device_manager.send_msg_to_device(device["id"], msg.get_raw()) != None:
                time.sleep(0.2) # Wait for status device to respond to poll
                return
            logger.warn(f"Retrying send GET message to id: {device['id']}")
            time.sleep(0.2)
        logger.error(f"Failed to send GET message to device:{device['type']} with uuid:{device['uuid']}")

    def set_device_param(self, uuid: list[int], parameter: str, new_val: str) -> bool:
        """
        Set the a parameter for the device
        """
        device = self.db_manager.search_device_in_db(uuid)
        if device is None:
            logger.error(f"Device with uuid:{uuid} not in DB!")
            return False
        class_obj = self.device_manager.get_supported_device(device["type"])
        if class_obj is None:
            return False
        if parameter not in class_obj.settable_parameters:
            return False

        # Update the set_status in the db
        if "set_status" not in device:
            device["set_status"] = {}
        device["set_status"][parameter] = new_val
        self.db_manager.update_device_in_db(device)
        # if parameter == "brightness":
        #     msg = HostMessage(self.db_manager.uuid, MSG_TYPES.SET, SetMessage(1, CHANGE_TYPES.SET, int(new_val)).get_raw())
        #     self.device_manager.send_msg_to_device(1, msg.get_raw())
        return True
