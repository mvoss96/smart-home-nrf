from nrf24USB import NRF24Device
from nrf24Smart import DeviceMessage, HostMessage, MSG_TYPES
import time
import random
from tinydb import TinyDB, Query


class Smart_Home:
    def __init__(self):
        self.db = TinyDB("db.json")
        self.device = NRF24Device("/dev/ttyUSB0", channel=101, address=0)

        # Create devices table, if it not already exists
        if "devices" not in self.db.tables():
            self.db.table("devices")
        self.devices_table = self.db.table("devices")

        # Create uuid if it not already exists
        Q = Query()
        search_result = self.db.search(Q.uuid.exists())
        if not search_result:
            self.uuid = self.generate_uuid()
            self.db.insert({"uuid": self.uuid})
            print(f"Generated and stored UUID: { self.uuid}")
        else:
            self.uuid = search_result[0]["uuid"]
            print(f"Read UUID: {self.uuid}")

    def generate_uuid(self):
        return [random.randint(0, 255) for _ in range(4)]

    def get_free_id(self):
        all_ids = set(device["id"] for device in self.devices_table.all())

        # Get smallest free id between 1 and 254:
        new_id = next((i for i in range(1, 255) if i not in all_ids), None)
        return new_id

    def init_new_device(self, msg):
        device_type = bytes(msg.DATA).decode(errors="ignore")
        print("New Device:", device_type, msg)

        # Check if a device with the UUID exists
        Q = Query()
        result = self.devices_table.search(Q.uuid == msg.UUID)
        if result:
            print("Device already in DB!")
            return

        # Get smallest free id between 1 and 254:
        new_id = self.get_free_id()
        if new_id is None:
            print("No free ID available")
            return
        print("New ID:", new_id)

        # Send new ID to device
        to_send_msg = HostMessage(uuid=self.uuid, msg_type=MSG_TYPES.INIT, data=[new_id])
        print(f"Sending new ID to {msg.ID} with data: {to_send_msg.get_raw()}")
        if self.device.send_msg(msg.ID, to_send_msg.get_raw()) == None:
            print(f"Failed to initialize device {device_type}")
            return

        # Add new Device
        self.devices_table.insert(
            {
                "uuid": msg.UUID,
                "id": new_id,
                "version": msg.FIRMWARE_VERSION,
                "battery": msg.BATTERY,
                "type": device_type,
            }
        )
        print("Device added!")

    def listen(self):
        data = self.device.get_message()
        if data:
            # print(data)
            msg = DeviceMessage(data)
            if not msg.is_valid:
                print("invalid message!")
                return
            if msg.MSG_TYPE == MSG_TYPES.INIT.value:
                self.init_new_device(msg)
            else:
                print("other", msg)

        else:
            time.sleep(0.01)


if __name__ == "__main__":
    home = Smart_Home()

    # Listen for incoming Messages
    while True:
        home.listen()
