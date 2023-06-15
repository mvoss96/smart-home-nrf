import time

class DeviceStatus:
    def get_status(self):
        raise NotImplementedError()


class LedController3Channel(DeviceStatus):
    def __init__(self, data: list[int]):
        if len(data) != 5:
            print("Incompatible Status for LedController3Channel")
            return
        self.power = data[0]
        self.brightness = data[1]
        self.ch_1 = data[2]
        self.ch_2 = data[3]
        self.ch_3 = data[4]
        self.timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

    def get_status(self):
        return {
            "power": self.power,
            "brightness": self.brightness,
            "ch_1": self.ch_1,
            "ch_2": self.ch_2,
            "ch_3": self.ch_3,
            "timestamp": self.timestamp
        }
    

supported_devices = [LedController3Channel]

