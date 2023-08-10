from .message import DeviceMessage, HostMessage, MSG_TYPES, SetMessage, CHANGE_TYPES
from. devices import DeviceStatus
from .LedController3Channel import LedController3Channel
from .RotRemote import RotRemote

supported_devices = [LedController3Channel, RotRemote]
