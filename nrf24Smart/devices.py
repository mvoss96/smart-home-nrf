import time
from typing import Type, Optional
from nrf24Smart import SetMessage, CHANGE_TYPES


class DeviceStatus:
    def __init__(self, data: list[int]):
        raise NotImplementedError()

    def get_status(self) -> dict:
        raise NotImplementedError()

    @classmethod
    def set_parameter(cls, param: str, new_val: str) -> Optional[SetMessage]:
        raise NotImplementedError()

    @classmethod
    def parse_bool(cls, value: str) -> Optional[int]:
        if value.lower() in ["0", "null", "false", "off"]:
            return 0
        elif value.lower() in ["1", "one", "true", "on"]:
            return 1
        else:
            return None

    @classmethod
    def parse_to_byte(cls, value: str) -> Optional[int]:
        try:
            # Try to convert the value to an integer
            num = int(value)
            if 0 <= num <= 255:
                return num
            else:
                return None
        except ValueError:
            # Try to convert the value to a float
            try:
                float_num = float(value)
                if 0.0 <= float_num <= 1.0:
                    return int(float_num * 255)
                else:
                    return None
            except ValueError:
                # If it still fails, then it's not a valid input
                return None


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

    def get_status(self) -> dict:
        return {
            "power": self.power,
            "brightness": self.brightness,
            "ch_1": self.ch_1,
            "ch_2": self.ch_2,
            "ch_2": self.ch_3,
            "timestamp": self.timestamp,
        }

    @classmethod
    def set_parameter(cls, param: str, new_val: str) -> Optional[SetMessage]:
        if param == "power":
            index = 0
            data = cls.parse_bool(new_val)
        elif param == "brightness":
            index = 1
            data = cls.parse_to_byte(new_val)
        elif param == "ch_1":
            index = 2
            data = cls.parse_to_byte(new_val)
        elif param == "ch_2":
            index = 3
            data = cls.parse_to_byte(new_val)
        elif param == "ch_3":
            index = 4
            data = cls.parse_to_byte(new_val)
        else:
            return None
        if data is None:
            return None
        return SetMessage(index, CHANGE_TYPES.SET, data)


supported_devices = [LedController3Channel]
