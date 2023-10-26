import time
import logging
from typing import Optional
from .devices import DeviceStatus, SetMessage, CHANGE_TYPES


class LedController3Ch(DeviceStatus):
    settable_parameters = [
        "power",  # 0
        "brightness",  # 1
        "ch_1",  # 2
        "ch_2",  # 3
        "ch_3",  # 4
        "rgb",  # 5
        "output_power_limit",  # 6
        "status_interval",  # 7
        "cct",  # virtual
        "cct_mired",  # virtual
        "brightness_percent",  # virtual
    ]
    supported_versions = [1]
    min_cct = 2500
    max_cct = 6500
    mqtt_discovery_paramters = [
        ("light", "status"),
    ]

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
            "brightness_percent": 100 * self.brightness // 255,
            "ch_1": self.ch_1,
            "ch_2": self.ch_2,
            "ch_3": self.ch_3,
            "cct" : self.get_cct(self.ch_1, self.ch_2),
            "cct_mired" : self.get_cct_mired(self.ch_1, self.ch_2),
            "num_channels": self.num_channels,
            "power_scale": self.power_scale,
            "timestamp": self.timestamp,
        }
        return status
    
    @classmethod
    def get_cct(cls, ch_1, ch_2) -> int:
        mixing_factor = ((ch_2 - ch_1) / 255.0 + 1) / 2
        kelvin = int(cls.min_cct + (cls.max_cct - cls.min_cct) * mixing_factor)
        return kelvin
    
    @classmethod
    def get_cct_mired(cls, ch_1, ch_2):
        kelvin = cls.get_cct(ch_1, ch_2) 
        if kelvin <= 0:
            return 0 
        mired = int(1_000_000 / kelvin)
        return mired
    
    @classmethod
    def get_param(cls, parameter: str, status: dict) -> Optional[str]:
        if status is None:
            return None
        if parameter == "rgb":
            return f"{status.get('ch_1')},{status.get('ch_2')},{status.get('ch_3')}"
        elif parameter == "cct":
            ch_1 = status.get("ch_1")
            ch_2 = status.get("ch_2")
            if ch_1 is None or ch_2 is None:
                return None
            kelvin = cls.get_cct(ch_1, ch_2)
            return str(kelvin)
        return status.get(parameter)
    
    @classmethod
    def from_cct(cls, kelvin) -> tuple[int, int]:
        if kelvin > cls.max_cct or kelvin < cls.min_cct:
            logging.warning("parameter cct out of bounds")
            kelvin = max(cls.min_cct, min(kelvin, cls.max_cct))
        mixing_factor = (kelvin - cls.min_cct) / (cls.max_cct - cls.min_cct)
        ch_1 = int((1 - mixing_factor) * 255.0)
        ch_2 = int(mixing_factor * 255.0)
        return ch_1, ch_2

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
        elif param == "status_interval":
            data = cls.parse_byte(new_val)
        elif param == "cct":
            try:
                kelvin = int(new_val)
            except ValueError:
                return None
            ch_1, ch_2 = cls.from_cct(kelvin)
            index = cls.settable_parameters.index("rgb")
            data = [ch_1, ch_2, 0]
        elif param == "cct_mired":
            try:
                mired = int(new_val)
                kelvin = 1_000_000 / mired
            except (ValueError, ZeroDivisionError):
                return None
            ch_1, ch_2 = cls.from_cct(kelvin)
            index = cls.settable_parameters.index("rgb")
            data = [ch_1, ch_2, 0]

        elif param == "brightness_percent":
            index = index = cls.settable_parameters.index("brightness")
            data = cls.parse_byte(new_val)
            if data:
                data = round(data * 255 / 100)
        else:
            return None
        if data is None:
            return None
        return SetMessage(index, CHANGE_TYPES.SET, data)
