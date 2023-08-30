import time
import logging
from typing import Optional
from .devices import DeviceStatus, SetMessage, CHANGE_TYPES


class SensRemote(DeviceStatus):
    settable_parameters = [
        "target",  # 0
    ]
    supported_versions = [1]

    def __init__(self, data: list[int]):
        self.valid = False
        if len(data) != 13:
            raise ValueError(f"Incompatible Status for {self.__class__.__name__}")
        self.valid = True
        self.targetID = data[0]
        self.targetUUID = data[1:5]
        self.temperature = self.parse_to_float(data[5:9], round_value = 1)
        self.humidity = self.parse_to_float(data[9:13], round_value = 1)
        self.timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

    def get_status(self) -> dict:
        status = {
            "targetID": self.targetID,
            "targetUUID": self.targetUUID,
            "temperature": self.temperature,
            "humidity": self.humidity,
            "timestamp": self.timestamp,
        }
        return status

    @classmethod
    def create_set_message(cls, param: str, new_val: str) -> Optional[SetMessage]:
        index = cls.settable_parameters.index(param)
        if param == "target":  # 0 one byte ID and four bytes UUID
            data = cls.parse_bytes(new_val)
            if len(data) != 5:
                data = None
        else:
            return None
        if data is None:
            return None
        return SetMessage(index, CHANGE_TYPES.SET, data)
