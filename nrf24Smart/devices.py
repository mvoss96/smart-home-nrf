import time
from typing import Type, Optional
import struct
from nrf24Smart import SetMessage, CHANGE_TYPES


class DeviceStatus:
    settable_parameters = []

    def __init__(self, data: list[int]):
        raise NotImplementedError()

    def get_status(self) -> dict:
        raise NotImplementedError()

    @classmethod
    def create_set_message(cls, param: str, new_val: str) -> Optional[SetMessage]:
        raise NotImplementedError()

    @classmethod
    def parse_to_float(cls, value: list[int]) -> Optional[float]:
        try:
            # Convert list of ints to bytes
            bytes_object = bytes(value)
            # Unpack bytes to float
            single_float = struct.unpack("f", bytes_object)[0]
            return single_float
        except Exception:
            return None

    @classmethod
    def parse_to_int(cls, value: list[int]) -> Optional[int]:
        try:
            bytes_object = bytes(value)  # Convert list of ints to bytes
            single_int = struct.unpack("i", bytes_object)[0]  # Unpack bytes to int
            return single_int
        except Exception:
            return None

    @classmethod
    def parse_bool(cls, value: str) -> Optional[int]:
        if value.lower() in ["0", "null", "false", "off"]:
            return 0
        elif value.lower() in ["1", "one", "true", "on"]:
            return 1
        else:
            return None

    @classmethod
    def parse_int(cls, value: str) -> Optional[list[int]]:
        try:
            integer = int(value)  # Convert the string to an integer
            # Pack the integer into a byte array and convert each byte to an integer
            return [b for b in struct.pack(">I", integer)]
        except Exception:  # If the string cannot be converted to an integer
            return None

    @classmethod
    def parse_byte(cls, value: str) -> Optional[int]:
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

    @classmethod
    def parse_bytes(cls, value: str) -> list[int]:
        parsed_bytes = []
        for substr in value.split(","):
            b = cls.parse_byte(substr)
            if b == None:
                return []
            parsed_bytes.append(b)
        return parsed_bytes


class LedController3Channel(DeviceStatus):
    settable_parameters = ["power", "brightness", "ch_1", "ch_2", "ch_3", "rgb", "output_power_limit"]

    def __init__(self, data: list[int]):
        self.valid = False
        if len(data) != 9:
            raise ValueError("Incompatible Status for LedController3Channel")
        self.valid = True
        self.power = data[0]
        self.brightness = data[1]
        self.ch_1 = data[2]
        self.ch_2 = data[3]
        self.ch_3 = data[4]
        ps = self.parse_to_float(data[5:9])
        self.power_scale = round(ps, 2) if ps is not None else None
        self.timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

    def get_status(self) -> dict:
        status = {
            "power": self.power,
            "brightness": self.brightness,
            "ch_1": self.ch_1,
            "ch_2": self.ch_2,
            "ch_3": self.ch_3,
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


supported_devices = [LedController3Channel]
