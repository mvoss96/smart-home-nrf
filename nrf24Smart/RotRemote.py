import time
import logging
from typing import Optional
from .devices import DeviceStatus, SetMessage, CHANGE_TYPES


class RotRemote(DeviceStatus):
    settable_parameters = []
    supported_versions = [1]

    def __init__(self, data: list[int]):
        self.valid = False
        if len(data) != 5:
            raise ValueError(f"Incompatible Status for {self.__class__.__name__}")
        self.valid = True
        self.targetID = data[0]
        self.targetUUID = data[1:5]
        self.timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

    def get_status(self) -> dict:
        status = {
            "targetID": self.targetID,
            "targetUUID": self.targetUUID,
            "timestamp": self.timestamp,
        }
        return status