import time
from typing import Type, Optional
import struct
from nrf24Smart import SetMessage, CHANGE_TYPES


class DeviceStatus:
    settable_parameters = []
    supported_versions = []
    mqtt_discovery_paramters = []

    def __init__(self, data: list[int]):
        raise NotImplementedError()

    def get_status(self) -> dict:
        raise NotImplementedError()

    @classmethod
    def get_param(cls, parameter: str, status: dict) -> Optional[str]:
        raise NotImplementedError()

    @classmethod
    def create_set_message(cls, param: str, new_val: str) -> Optional[SetMessage]:
        raise NotImplementedError()

    @classmethod
    def parse_to_float(cls, value: list[int], round_value: int = 0) -> Optional[float]:
        try:
            # Convert list of ints to bytes
            bytes_object = bytes(value)
            # Unpack bytes to float
            single_float = struct.unpack("f", bytes_object)[0]
            if round_value != 0:
                return round(single_float, round_value)
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
