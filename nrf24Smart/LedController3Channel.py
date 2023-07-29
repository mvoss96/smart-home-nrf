import time
from typing import Optional
from .devices import DeviceStatus, SetMessage, CHANGE_TYPES


class LedController3Channel(DeviceStatus):
    settable_parameters = ["power", "brightness", "ch_1", "ch_2", "ch_3", "rgb", "num_channels", "output_power_limit"]
    supported_versions = [1]

    def __init__(self, data: list[int]):
        self.valid = False
        if len(data) != 10:
            raise ValueError("Incompatible Status for LedController3Channel")
        self.valid = True
        self.power = data[0]
        self.brightness = data[1]
        self.ch_1 = data[2]
        self.ch_2 = data[3]
        self.ch_3 = data[4]
        self.num_channels = data[5]
        ps = self.parse_to_float(data[6:10])
        self.power_scale = round(ps, 2) if ps is not None else None
        self.timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

    def get_status(self) -> dict:
        status = {
            "power": self.power,
            "brightness": self.brightness,
            "ch_1": self.ch_1,
            "ch_2": self.ch_2,
            "ch_3": self.ch_3,
            "num_channels": self.num_channels,
            "power_scale": self.power_scale,
            "timestamp": self.timestamp,
        }
        return status

    @classmethod
    def create_set_message(cls, param: str, new_val: str) -> Optional[SetMessage]:
        index = cls.settable_parameters.index(param)
        if param == "power":
            data = cls.parse_bool(new_val)
        elif param == "brightness":
            data = cls.parse_byte(new_val)
        elif param == "ch_1":
            data = cls.parse_byte(new_val)
        elif param == "ch_2":
            data = cls.parse_byte(new_val)
        elif param == "ch_3":
            data = cls.parse_byte(new_val)
        elif param == "rgb":
            data = cls.parse_bytes(new_val)
            if len(data) != 3:
                data = None
        elif param == "output_power_limit":
            data = cls.parse_int(new_val)
            print("limit:", new_val, data)
        else:
            return None
        if data is None:
            return None
        return SetMessage(index, CHANGE_TYPES.SET, data)
