from nrf24Smart import DeviceMessage, HostMessage, MSG_TYPES
import time
from datetime import datetime
from DeviceManager import DeviceManager
from Logger import setup_logger

logger = setup_logger()


class CommunicationManager:
    def __init__(self, device_manager: DeviceManager):
        # Initializing the last_update_time
        self.last_update_time = time.time()

        # Reference to the DeviceManager instance to handle device operations
        self.device_manager = device_manager
        self.db_manager = self.device_manager.db_manager

        self.parameter_buffer = {}

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
            logger.error(err)

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
                    logger.warning(f"invalid message! {msg.raw_data}")
                    continue
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
            
            for device in self.db_manager.get_all_devices():
                uuid = device["uuid"]
                uuid_string = str(uuid)
                if uuid_string not in self.parameter_buffer:
                    #logger.error(f"set_status missing from device {device['type']}")
                    continue
                if (class_obj := self.device_manager.get_supported_device(device["type"])) == None:
                    logger.error(f"Database contains not supported device {device['type']}")
                    continue

                keys_updated = set()
                dict_copy = dict(self.parameter_buffer[uuid_string])

                for key, value in dict_copy.items():
                    if (set_message := class_obj.create_set_message(key, value)) == None:
                        logger.error(f"set_status contains not supported parameter {key}: {value}")
                        keys_updated.add(key)
                        continue

                    msg = HostMessage(
                        uuid=self.db_manager.uuid, msg_type=MSG_TYPES.SET, data=set_message.get_raw()
                    )
                    logger.info(f"sending SET {key} {value} to device {uuid}")
                    res = self.device_manager.send_msg_to_device(device["id"], msg.get_raw())
                    if res == None:
                        logger.error(
                            f"Failed to send SET message to device:{device['type']} with uuid:{device['uuid']}!"
                        )
                    else:
                        keys_updated.add(key)

                    #time.sleep(0.2)

                self.poll_device(uuid) # Update Device Status

                for key in keys_updated:
                    # Only remove if values have not changed:
                    if dict_copy.get(key) == self.parameter_buffer[uuid_string].get(key):
                        #print(f"delete {dict_copy.get(key)} {self.parameter_buffer[uuid_string].get(key)}")
                        del self.parameter_buffer[uuid_string][key]
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
        logger.info(f"poll device {uuid}")
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
        logger.info(f"set {uuid} parameter: {parameter} to new_val: {new_val}")
        device = self.db_manager.search_device_in_db(uuid)
        if device is None:
            logger.error(f"Device with uuid:{uuid} not in DB!")
            return False
        class_obj = self.device_manager.get_supported_device(device["type"])
        if class_obj is None:
            logger.warning(f"{device['type']} not supported")
            return False
        if parameter not in class_obj.settable_parameters:
            logger.warning(f"parameter {parameter} not supported")
            return False
        
        # # Update the set_status in the db
        # if "set_status" not in device:
        #     device["set_status"] = {}

        # device["set_status"][parameter] = new_val
        # logger.info(f"update set_param in db: {device['set_status'][parameter]}")
        # self.db_manager.update_device_in_db(device)
        uuid_string = str(uuid)
        if uuid_string not in self.parameter_buffer:
            self.parameter_buffer[uuid_string] = {}
        self.parameter_buffer[uuid_string][parameter] = new_val
        return True
