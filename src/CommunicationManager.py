from nrf24Smart import DeviceMessage, HostMessage, RemoteMessage, MSG_TYPES
import time
from datetime import datetime
from src.DeviceManager import DeviceManager
from src.Logger import setup_logger
from threading import Event
from typing import Optional
from collections import deque
import queue

logger = setup_logger()


class CommunicationManager:
    def __init__(self, device_manager: DeviceManager, shutdown_flag: Event):
        # Initializing the last_update_time
        self.last_update_time = time.time()

        # Set shutdown_flag
        self.shutdown_flag = shutdown_flag

        # Reference to the DeviceManager instance to handle device operations
        self.device_manager = device_manager
        self.db_manager = self.device_manager.db_manager

        # Internal Buffer for puffering status changes
        self.parameter_buffer = {}
        self.failed_sends = {}

        # Internal Dict for storing msg_nums to calculate a connection health
        self.max_msg_num_length = 20
        self.msg_nums = {}

        # Devices where we are waiting for an ack message
        self.wait_for_status_acks: bool = True
        self.wait_for_status = set()

        # Queque to notify the mqttManager about events
        self.event_queue = queue.Queue()

    def calc_missing(self, lst) -> int:
        # Create a complete set from the smallest to the largest number in the list
        complete_set = set(range(min(lst), max(lst) + 1))

        # Convert the original list to a set
        lst_set = set(lst)

        # Find the difference between the complete set and the original set
        missing_set = complete_set - lst_set

        # Return the number of missing entries
        return len(missing_set)

    def update_connection_health(self, uuid: list[int], msg_num: int):
        """
        Updates the Connection Status by calcualting the missed messages
        """
        # Return early if device not in db
        if self.db_manager.search_device_in_db(uuid) is None:
            return

        uuid_string = str(uuid)

        # Check if device_id is in msg_nums
        if uuid_string not in self.msg_nums:
            # If uuid_string is not in msg_nums, initialize it
            logger.debug(f"initialize id {uuid_string}")
            self.msg_nums[uuid_string] = deque(maxlen=self.max_msg_num_length)
            self.msg_nums[uuid_string].append(msg_num)

        # If msg_num either cycles back to 0 or the device resets, reset the list
        elif msg_num == 0 or msg_num < max(self.msg_nums[uuid_string]):
            logger.debug("reset list")
            self.msg_nums[uuid_string].clear()
            self.msg_nums[uuid_string].append(0)

        elif msg_num in self.msg_nums[uuid_string]:
            logger.warning(f"Repeated msg_num {msg_num} in {self.msg_nums[uuid_string]} for device {uuid}")

        # Add the msg_num to the list for uuid_string
        else:
            self.msg_nums[uuid_string].append(msg_num)

        # Calculate the connection health
        num_missing = self.calc_missing(self.msg_nums[uuid_string])
        connection_health = 1 - num_missing / (len(self.msg_nums[uuid_string]) + num_missing)
        self.db_manager.update_connection_health(uuid, connection_health)

    def check_device_message(self, msg: DeviceMessage | RemoteMessage) -> Optional[tuple]:
        """
        Check that the reported dvice exists and has a supported firmware
        Returns the device and the class_obj, or None if an error occured
        """
        device = self.db_manager.search_device_in_db(msg.UUID)
        if device == None:
            logger.warning(f"Device with uuid:{msg.UUID} not in DB!")
            return None

        # Check if the class exists in the supported_devices list
        class_obj = self.device_manager.get_supported_device(device["type"])
        if class_obj == None:
            logger.warning(f"Unsupported Device of type:{device['type']} in DB!")
            return None

        # Check that id and uuid match the db
        if msg.ID != device.get("id"):
            logger.warning(
                f"Mismatched ID and UUID! Device with UUID:{msg.UUID} reports ID:{msg.ID} instead of {device.get('id')}"
            )
            return None

        if isinstance(msg, DeviceMessage):
            # Check the firmware Version
            if msg.FIRMWARE_VERSION not in class_obj.supported_versions:
                logger.error(
                    f"Device with UUID {msg.UUID} reports unsupported Firmware Version {msg.FIRMWARE_VERSION}"
                )
            if msg.FIRMWARE_VERSION != device.get("version"):
                logger.warning(
                    f"Device with UUID {msg.UUID} changed Firmware Version from {device.get('version')} to {msg.FIRMWARE_VERSION}"
                )

            device["version"] = msg.FIRMWARE_VERSION
            device["status_interval"] = msg.STATUS_INTERVAL
        return device, class_obj

    def handle_status_message(self, msg: DeviceMessage):
        """
        Updates the status of a device given a DeviceMessage.
        The method fetches the device from the database, checks its type and updates its status.
        """
        ret = self.check_device_message(msg)
        if ret:
            device, class_obj = ret
        else:
            return

        if msg.MSG_TYPE == MSG_TYPES.OK.value and msg.ID in self.wait_for_status:
            self.wait_for_status.remove(msg.ID)
            # print("removed ID:", msg.ID)

        # Create an instance of the class
        try:
            instance = class_obj(msg.DATA)
            # Update the status key for the device using the device variable
            if device["battery_powered"]:
                device["battery_level"] = msg.BATTERY
                device["battery_percent"] = round(msg.BATTERY / 2.55)
            device["status"] = instance.get_status()
            device["last_seen"] = time.strftime("%Y-%m-%d %H:%M:%S")
            self.db_manager.update_device_in_db(device)
        except Exception as err:
            logger.error(err)

    def handle_remote_message(self, msg: RemoteMessage):
        """
        Handles Remote messages address to the server. They are then forwarded to the MQTT interface
        """
        ret = self.check_device_message(msg)
        if ret:
            device, class_obj = ret
        else:
            return

        event = "unknow"
        if hasattr(class_obj, "get_remote_event") and callable(getattr(class_obj, "get_remote_event")):
            event = class_obj.get_remote_event(msg.LAYER, msg.VALUE)
        self.event_queue.put((msg.UUID, event))

    def handle_boot_message(self, msg: DeviceMessage):
        """
        Checks the BOOT message from a device.
        It validates that the UUID of the server corresponds to the stored UUID reported by the Device
        """
        logger.info(f"BOOT message from device:{msg.UUID}")

    def handle_init_mesage(self, msg: DeviceMessage):
        """
        Initializes a new Device with the device_manager
        """
        self.device_manager.init_new_device(msg)

    def listen(self):
        """
        Listens for incoming messages.
        If a message is available, it processes the message according to its type.
        """
        while not self.shutdown_flag.is_set():
            time.sleep(0.01)
            data = self.device_manager.get_device_message()
            if not data:
                continue
            if len(data) < 6:
                logger.warning("Message from device to short")
                continue

            if data[5] == MSG_TYPES.REMOTE.value:
                msg = RemoteMessage(data)
                if not msg.is_valid:
                    logger.warning(f"invalid RemoteMessage! {msg.raw_data}")
                    continue
                self.handle_remote_message(msg)
            else:
                msg = DeviceMessage(data)
                if not msg.is_valid:
                    logger.warning(f"invalid message! {msg.raw_data}")
                    continue

                uuid_string = str(msg.UUID)
                if uuid_string in self.msg_nums and msg.MSG_NUM == self.msg_nums[uuid_string][-1]:
                    logger.warning(f"Ignore msg with repeated msg_num {msg.MSG_NUM} for {uuid_string}")
                    continue

                self.update_connection_health(msg.UUID, msg.MSG_NUM)
                if msg.MSG_TYPE == MSG_TYPES.INIT.value:
                    self.handle_init_mesage(msg)
                elif msg.MSG_TYPE == MSG_TYPES.BOOT.value:
                    self.handle_boot_message(msg)
                elif msg.MSG_TYPE in (MSG_TYPES.STATUS.value, MSG_TYPES.OK.value):
                    self.handle_status_message(msg)
                else:
                    logger.warning(f"->Unknown DeviceMessage {msg}")

        logger.info("Stopped listen")

    def update_all_devices(self):
        """
        Send any pending status changes in set_status to the devices.
        """
        while not self.shutdown_flag.is_set():
            keys_updated = []
            for device in self.db_manager.get_all_devices():
                uuid = device["uuid"]
                id = device["id"]
                uuid_string = str(uuid)
                keys_unsupported = set()

                if (class_obj := self.device_manager.get_supported_device(device["type"])) == None:
                    logger.error(f"Database contains not supported device {device['type']}")
                    continue

                if uuid_string in self.parameter_buffer and self.parameter_buffer[uuid_string] != {}:
                    dict_copy = dict(self.parameter_buffer[uuid_string])

                    for key, value in dict_copy.items():
                        if (set_message := class_obj.create_set_message(key, value)) == None:
                            logger.error(f"set_status contains not supported parameter {key}: {value}")
                            keys_unsupported.add(key)
                            continue

                        msg = HostMessage(
                            uuid=self.db_manager.uuid, msg_type=MSG_TYPES.SET, data=set_message.get_raw()
                        )
                        logger.info(f"sending SET {key} {value} to device {uuid}")
                        res = self.device_manager.send_msg_to_device(id, msg.get_raw())
                        if res == None:
                            logger.info(
                                f"Failed to send SET message to device:{device['type']} with uuid:{device['uuid']}!"
                            )
                            if uuid_string not in self.failed_sends:
                                self.failed_sends[uuid_string] = time.time()
                            elif time.time() - self.failed_sends[uuid_string] > 5:
                                logger.error(
                                    f"Timeout for SET message to device:{device['type']} with uuid:{device['uuid']}!"
                                )
                                del self.failed_sends[uuid_string]
                                keys_unsupported.add(key)
                        else:
                            if uuid_string in self.failed_sends:
                                del self.failed_sends[uuid_string]
                            if self.wait_for_status_acks:
                                self.wait_for_status.add(id)
                            keys_updated.append((id, uuid_string, key, value))
                        time.sleep(0.05)

                    for key in keys_unsupported:
                        # Only remove if values have not changed:
                        if dict_copy.get(key) == self.parameter_buffer[uuid_string].get(key):
                            print(f"Remove {dict_copy.get(key)} {self.parameter_buffer[uuid_string].get(key)}")
                            del self.parameter_buffer[uuid_string][key]
                    # self.db_manager.update_device_in_db(device)

            time.sleep(0.2)
            for entry in keys_updated:
                (id, uuid_string, key, value) = entry
                device = self.db_manager.search_device_in_db_by_id(id)
                if device is None:
                    continue
                # Only remove if values have not changed:
                for i in range(5):
                    if value == self.parameter_buffer[uuid_string].get(key):
                        #print("wait for status from id", id)
                        if id not in self.wait_for_status:
                            # print(f"delete {key}")
                            del self.parameter_buffer[uuid_string][key]
                            # self.db_manager.update_device_in_db(device)
                            keys_updated.remove(entry)
                            break
                        time.sleep(0.1)
                if id in self.wait_for_status:
                    logger.warning(f"did not receive status update in time from device {uuid_string}")

        logger.info("Stopped update_all_devices")

    def get_device_param(self, uuid, parameter):
        """
        Get a parameter for the device
        """
        logger.debug(f"get {uuid} parameter: {parameter}")
        if (device := self.db_manager.search_device_in_db(uuid)) is None:
            logger.error(f"Device with uuid:{uuid} not in DB!")
            return None
        if (class_obj := self.device_manager.get_supported_device(device["type"])) is None:
            logger.warning(f"{device['type']} not supported")
            return None
        if (status := device.get("status")) is None:
            logger.error(f"Device with uuid:{uuid} does not have a status!")
            return None
        return class_obj.get_param(parameter, status)

    def set_device_param(self, uuid: list[int], parameter: str, new_val: str) -> bool:
        """
        Set the a parameter for the device
        """
        logger.info(f"set {uuid} parameter: {parameter} to new_val: {new_val}")
        if (device := self.db_manager.search_device_in_db(uuid)) is None:
            logger.error(f"Device with uuid:{uuid} not in DB!")
            return False
        if (class_obj := self.device_manager.get_supported_device(device["type"])) is None:
            logger.warning(f"{device['type']} not supported")
            return False
        if parameter not in class_obj.settable_parameters:
            logger.warning(f"Setting parameter {parameter} not supported")
            return False

        uuid_string = str(uuid)
        if uuid_string not in self.parameter_buffer:
            self.parameter_buffer[uuid_string] = {}
        self.parameter_buffer[uuid_string][parameter] = new_val
        return True

    def get_event(self):
        return self.event_queue.get() if not self.event_queue.empty() else None
