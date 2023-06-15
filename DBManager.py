from tinydb import TinyDB, Query
import random
import logging

# Configure the root logger
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class DBManager:
    def __init__(self):
        # Initialize the TinyDB instance
        self.db = TinyDB("db2.json", indent=4)

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
        all_ids = set(device["id"] for device in self.devices_table.all())
        new_id = next((i for i in range(1, 255) if i not in all_ids), None)
        return new_id

    def search_device_in_db(self, uuid: list[int]):
        """
        Search for a device in the devices table using the given UUID.
        Returns the search result.
        """
        Q = Query()
        result = self.devices_table.search(Q.uuid == uuid)
        return result[0] if len(result) > 0 else None
    
    def search_device_in_db_by_id(self, device_id: int):
        """
        Search for a device in the devices table using the given ID.
        Returns the search result.
        """
        Q = Query()
        result = self.devices_table.search(Q.id == device_id)
        return result[0] if len(result) > 0 else None

    def get_all_devices(self):
        """
        Returns all devices in the database.
        """
        return self.devices_table.all()

    def add_device_to_db(self, device_dict: dict):
        """
        Inserts a new device into the devices table.
        The new device's data is given as a dictionary.
        """
        self.devices_table.insert(device_dict)
        logger.info(f"Device {device_dict['type']} added!")

    def update_device_in_db(self, device_dict: dict):
        """
        Updates a device's information in the database.
        """
        Q = Query()
        self.devices_table.update(device_dict, Q.uuid == device_dict['uuid'])

    def remove_device_from_db(self, device_id: int):
        """
        Remove a device from the devices table using the given ID.
        """
        logger.info(f"Remove Device with id {device_id} from db")
        Q = Query()
        self.devices_table.remove(Q.id == device_id)
        
