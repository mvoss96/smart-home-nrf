from enum import Enum


class MSG_TYPES(Enum):
    INIT = 0
    BOOT = 1
    OK = 2
    SET = 3
    GET = 4
    RESET = 5
    STATUS = 6

class CHANGE_TYPES(Enum):
   SET = 0
   TOGGLE = 1
   INCREASE = 2
   DECREASE = 3


class SetMessage:
    def __init__(self, index: int, changeType: CHANGE_TYPES, value : list[int] = [0]):
        self.varIndex = index
        self.changeType = changeType 
        self.valueSize= len(value)
        self.newValue = value

    def get_raw(self) -> list[int]:
        return [self.varIndex] + [self.changeType.value] + [self.valueSize] + self.newValue
    
    def __str__(self) -> str:
        return (
            f"SetMessage varIndex:{self.varIndex} changeType:{self.changeType} valueSize:{self.valueSize} newValue:{self.newValue}")
    


class DeviceMessage:
    def __init__(self, raw_data) -> None:
        if len(raw_data) < 8:
            print("DeviceMessage must be at least 8 bytes long")
            self.is_valid = False
            return
        self.raw_data = raw_data
        self.ID = self.raw_data[0]
        self.UUID = self.raw_data[1:5]
        self.MSG_TYPE = self.raw_data[5]
        self.FIRMWARE_VERSION = self.raw_data[6]
        self.BATTERY = self.raw_data[7]
        self.DATA = self.raw_data[8:-2]
        self.CHECKSUM = (self.raw_data[-2] << 8) | self.raw_data[-1]  # MSB, LSB
        self.is_valid = self.CHECKSUM == sum(
            self.raw_data[:-2]
        )  # Calculate the checksum from the data (excluding checksum bytes)

    def __str__(self):
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
