from .message import DeviceMessage, HostMessage, MSG_TYPES, SetMessage, CHANGE_TYPES
from. devices import DeviceStatus
from .LedController3Ch import LedController3Ch
from .RotRemote import RotRemote
from .SensRemote import SensRemote

supported_devices = [LedController3Ch, RotRemote, SensRemote]
