from nrf24USB import NRF24Device
from nrf24Smart import DeviceMessage, HostMessage, SetMessage, CHANGE_TYPES, MSG_TYPES, supported_devices
import time
import random
from tinydb import TinyDB, Query


class Smart_Home:
    def __init__(self):
        self.db = TinyDB("db.json", indent=4)
        self.device = NRF24Device("/dev/ttyUSB0", channel=101, address=0)
        self.last_update_time = time.time()

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

    def init_new_device(self, msg: DeviceMessage):
        device_type = bytes(msg.DATA).decode(errors="ignore")
        print("New Device:", device_type, msg)

        # Check if the class exists in the supported_devices list
        if not any(hasattr(cls, "__name__") and cls.__name__ == device_type for cls in supported_devices):
            print("Device type not supported!")
            return

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
                "battery_powered": msg.BATTERY != 0,
                "battery_level": 255,
                "type": device_type,
                "last_seen": time.strftime("%Y-%m-%d %H:%M:%S"),
            }
        )
        print(f"Device {device_type} added!")

    def check_boot_msg(self, msg: DeviceMessage):
        print(f"BOOT message from device:{msg.UUID}")
        if msg.DATA != self.uuid:
            print(
                f"Mismatched UUID! Device with uuid:{msg.UUID} reports server uuid:{msg.DATA} instead of {self.uuid}"
            )

    def update_status(self, msg: DeviceMessage):
        Q = Query()
        result = self.devices_table.search(Q.uuid == msg.UUID)
        if len(result) == 0:
            print(f"Device with uuid:{msg.UUID} not in DB!")
            return
        device = result[0]
        # Check if the class exists in the supported_devices list
        if not any(hasattr(cls, "__name__") and cls.__name__ == device["type"] for cls in supported_devices):
            f"Unsupported Device of type:{device['type']} in DB!"
            return

        if device["battery_powered"]:
            device["battery_level"] = msg.BATTERY

        # Get the class object by name
        class_obj = next((cls for cls in supported_devices if cls.__name__ == device["type"]))
        # Create an instance of the class
        instance = class_obj(msg.DATA)
        # print(instance.get_status())
        # Update the status key for the device using the device variable
        device["status"] = instance.get_status()
        device["last_seen"] = time.strftime("%Y-%m-%d %H:%M:%S")
        self.devices_table.update(device, Q.uuid == msg.UUID)

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
            elif msg.MSG_TYPE == MSG_TYPES.BOOT.value:
                self.check_boot_msg(msg)
            elif msg.MSG_TYPE == MSG_TYPES.STATUS.value:
                self.update_status(msg)
            else:
                print("->", msg)

        else:
            time.sleep(0.01)

    def update_device(self, uuid: list[int]):
        Q = Query()
        result = self.devices_table.search(Q.uuid == uuid)
        if len(result) == 0:
            print(f"Device with uuid:{uuid} not in DB!")
            return
        device = result[0]
        msg = HostMessage(uuid=self.uuid, msg_type=MSG_TYPES.GET, data=[])
        if self.device.send_msg(device["id"], msg.get_raw()) == None:
            print(f"Failed to send GET message to device:{device['type']} with uuid:{device['uuid']}!")

    def update_all_devices(self):
        current_time = time.time()
        if current_time - self.last_update_time < 2.0:  # less than one second has passed
            return  # Return without doing anything
        for item in self.devices_table:
            # print(f"update status of device:{item['type']} uuid:{item['uuid']}")
            self.update_device(item["uuid"])

        self.last_update_time = current_time  # Update last_run_time to current_time

    def send_test(self):
        current_time = time.time()
        if current_time - self.last_update_time < 2.0:
            return
        device = self.devices_table.all()[0]
        msg = HostMessage(
            uuid=self.uuid,
            msg_type=MSG_TYPES.SET,
            data=SetMessage(index=1, changeType=CHANGE_TYPES.DECREASE, value=[5]).get_raw(),
        )
        print(self.device.send_msg(1, msg.get_raw()))
        self.update_device(device["uuid"])
        self.last_update_time = current_time


if __name__ == "__main__":
    home = Smart_Home()

    # Listen for incoming Messages
    while True:
        home.listen()
        # home.update_devices()
        home.send_test()
        # to_send_msg = HostMessage(uuid=[1,2,3,4], msg_type=MSG_TYPES.RESET, data=[0])
        # print(home.device.send_msg(1, to_send_msg.get_raw()))
        # time.sleep(1)
