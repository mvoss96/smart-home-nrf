from tinydb import TinyDB, Query
import random
from threading import Lock
from src.Logger import setup_logger

logger = setup_logger()


class DBManager:
    def __init__(self):
        # Initialize the TinyDB instance
        self.db = TinyDB("db.json", indent=4)

        # Initialize the lock
        self.db_lock = Lock()

        # Initialize the devices_table attribute by calling the initialize_devices_table method
        self.devices_table = self.initialize_devices_table()

        # Initialize the uuid attribute by calling the initialize_uuid method
        self.uuid = self.initialize_uuid()

    def initialize_devices_table(self):
        """
        Check if a "devices" table exists in the database.
        If not, it creates a new table named "devices".
        Then it returns this "devices" table.
        """
        if "devices" not in self.db.tables():
            self.db.table("devices")
        return self.db.table("devices")

    def initialize_uuid(self):
        """
        Check if a uuid field exists in the database.
        If not, it generates a new uuid and inserts it into the database.
        Then it returns this uuid.
        """
        Q = Query()
        with self.db_lock:
            search_result = self.db.search(Q.uuid.exists())
            if not search_result:
                uuid = self.generate_uuid()
                self.db.insert({"uuid": uuid})
                logger.info(f"Generated and stored UUID: { uuid}")
                return uuid
            else:
                uuid = search_result[0]["uuid"]
                logger.info(f"Read UUID: {uuid}")
                return uuid

    def generate_uuid(self):
        """
        Generate a new UUID consisting of 4 random numbers each between 0 and 255.
        Return this new UUID.
        """
        return [random.randint(0, 255) for _ in range(4)]

    def get_free_id(self):
        """
        Search through all device IDs in the devices table to find an unused ID.
        Returns the smallest unused ID.
        If no unused ID is found, it returns None.
        """
        with self.db_lock:
            all_ids = set(device["id"] for device in self.devices_table.all())
        new_id = next((i for i in range(1, 255) if i not in all_ids), None)
        return new_id

    def set_http_password(self, pw):
        "Sets the http_password. If an entry in the database already exists it gets updated."
        Q = Query()
        existing_record = self.db.search(Q.http_password.exists())
        if existing_record:
            if existing_record[0]["http_password"] != pw:
                self.db.update({"http_password": pw}, Q.http_password.exists())
                logger.info("Updated http_password")
        else:
            self.db.insert({"http_password": pw})
            logger.info("Set http_password")

    def check_http_password(self, pw):
        """
        Returns wether the provided password corresponds to the one in the database
        """
        Q = Query()
        with self.db_lock:
            search_result = self.db.search(Q.http_password.exists())
            if not search_result:
                logger.warn(f"No HTTP password set!")
                return False
            else:
                return search_result[0]["http_password"] == pw

    def search_device_in_db(self, uuid: list[int]):
        """
        Search for a device in the devices table using the given UUID.
        Returns the search result.
        """
        try:
            with self.db_lock:
                Q = Query()
                result = self.devices_table.search(Q.uuid == uuid)
            return result[0] if len(result) > 0 else None
        except Exception as e:
            logger.error(f"Error occurred while searching device by UUID: {e}")
            return None

    def search_device_in_db_by_id(self, device_id: int):
        """
        Search for a device in the devices table using the given ID.
        Returns the search result.
        """
        Q = Query()
        try:
            with self.db_lock:
                result = self.devices_table.search(Q.id == device_id)
            return result[0] if len(result) > 0 else None
        except Exception as e:
            logger.error(f"Error occurred while searching device by ID: {e}")
            return None

    def get_all_devices(self):
        """
        Returns all devices in the database.
        """
        try:
            with self.db_lock:
                return self.devices_table.all()
        except Exception as e:
            logger.error(f"Error occurred while getting all devices: {e}")
            return []

    def add_device_to_db(self, device_dict: dict):
        """
        Inserts a new device into the devices table.
        The new device's data is given as a dictionary.
        """
        try:
            with self.db_lock:
                self.devices_table.insert(device_dict)
            logger.info(f"Device {device_dict['type']} added!")
        except Exception as e:
            logger.error(f"Unexpected error while adding device to DB: {e}")

    def update_device_in_db(self, device_dict: dict):
        """
        Updates a device's information in the database.
        """
        Q = Query()
        try:
            with self.db_lock:
                self.devices_table.update(device_dict, Q.uuid == device_dict["uuid"])
        except Exception as e:
            logger.error(f"Unexpected error while updating device in DB: {e}")

    def remove_device_from_db(self, device_uuid: list[int]):
        """
        Remove a device from the devices table using the given UUID.
        """
        logger.info(f"Remove Device with uuid {device_uuid} from db")
        Q = Query()
        try:
            with self.db_lock:
                self.devices_table.remove(Q.uuid == device_uuid)
        except Exception as e:
            logger.error(f"Unexpected error while removing device in DB: {e}")

    def update_device_name(self, device_uuid: list[int], new_name: str):
        """
        Change the name of a Device using the given UUID
        """
        logger.info(f"Changing Name of Device with uuid {device_uuid} to {new_name}")
        Q = Query()
        try:
            # Find the device with the given UUID
            device = self.search_device_in_db(device_uuid)
            if device:
                device["name"] = new_name
                self.update_device_in_db(device)
        except Exception as e:
            logger.error(f"Unexpected error while renaming device in DB: {e}")
