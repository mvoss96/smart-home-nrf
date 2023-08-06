from enum import Enum
import logging

class MSG_TYPES(Enum):
    ERROR = 0
    INIT = 1
    BOOT = 2
    OK = 3
    SET = 4
    GET = 5
    RESET = 6
    STATUS = 7

class CHANGE_TYPES(Enum):
   INVALID = 0
   SET = 1
   TOGGLE = 2
   INCREASE = 3
   DECREASE = 4


class SetMessage:
    def __init__(self, index: int, changeType: CHANGE_TYPES, value : list[int] | int = [0]):
        self.varIndex = index
        self.changeType = changeType 
        if not isinstance(value, list):
            value = [value]
        if not all(isinstance(item, int) for item in value):
            logging.error("value list must only contain ints")
            value = [0]
        self.valueSize= len(value)
        self.newValue = value

    def get_raw(self) -> list[int]:
        return [self.varIndex] + [self.changeType.value] + [self.valueSize] + self.newValue
    
    def __str__(self) -> str:
        return (
            f"SetMessage varIndex:{self.varIndex} changeType:{self.changeType} valueSize:{self.valueSize} newValue:{self.newValue}")
    


class DeviceMessage:
    def __init__(self, raw_data) -> None:
        self.raw_data = raw_data
        if len(self.raw_data) < 8:
            print("DeviceMessage must be at least 8 bytes long")
            self.is_valid = False
            return
        self.ID = self.raw_data[0]
        self.UUID = self.raw_data[1:5]
        self.MSG_TYPE = self.raw_data[5]
        self.FIRMWARE_VERSION = self.raw_data[6]
        self.BATTERY = self.raw_data[7]
        self.DATA = self.raw_data[8:-2]
        self.CHECKSUM = (self.raw_data[-2] << 8) | self.raw_data[-1]  # MSB, LSB
        # Calculate the checksum from the data (excluding checksum bytes)
        self.is_valid = self.CHECKSUM == sum(self.raw_data[:-2])  

    def __str__(self) -> str:
        return (
            f"DeviceMessage ID:{self.ID} UUID:{':'.join(f'{byte:02X}' for byte in self.UUID)} TYPE: {MSG_TYPES(self.MSG_TYPE)} FW:{self.FIRMWARE_VERSION} "
            + f"BATTERY:{self.BATTERY} DATA:{':'.join(f'{byte:02X}' for byte in self.DATA)} VALID:{self.is_valid}"
        )


class HostMessage:
    def __init__(self, uuid: list[int], msg_type: MSG_TYPES, data: list[int]) -> None:
        self.ID = 0
        self.UUID = uuid
        self.MSG_TYPE = msg_type.value
        self.DATA = data
        self.CHECKSUM = self.calc_checksum()

    def calc_checksum(self) -> list[int]:
        # Calculate the checksum from the data (excluding checksum bytes)
        checksum = sum([self.ID] + self.UUID + [self.MSG_TYPE] + self.DATA)
        #print(f"checksum: {checksum}")
        # Return it as a combination of MSB and LSB
        return [checksum >> 8 & 0xFF, checksum & 0xFF]

    def get_raw(self) -> list[int]:
        return [self.ID] + self.UUID + [self.MSG_TYPE] + self.DATA + self.CHECKSUM

    def __str__(self) -> str:
        return (
            f"HostMessage ID:{self.ID} UUID:{':'.join(f'{byte:02X}' for byte in self.UUID)} TYPE: {MSG_TYPES(self.MSG_TYPE)} "
            + f"DATA:{':'.join(f'{byte:02X}' for byte in self.DATA)} CHECKSUM:{self.CHECKSUM[0]:02X}{self.CHECKSUM[1]:02X}"
        )
